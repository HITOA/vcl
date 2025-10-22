#include <VCL/CodeGen/CodeGenFunction.hpp>

#include <VCL/CodeGen/CodeGenModule.hpp>



llvm::Value* VCL::CodeGenFunction::GenerateExpr(Expr* expr) {
    switch (expr->GetExprClass()) {
        case Expr::NumericLiteralExprClass:
            return GenerateNumericLiteralExpr((NumericLiteralExpr*)expr);
        case Expr::DerefExprClass:
            return GenerateDerefExpr((DerefExpr*)expr);
        case Expr::DeclRefExprClass:
            return GenerateDeclRefExpr((DeclRefExpr*)expr);
        case Expr::CastExprClass:
            return GenerateCastExpr((CastExpr*)expr);
        case Expr::SplatExprClass:
            return GenerateSplatExpr((SplatExpr*)expr);
        case Expr::BinaryExprClass:
            return GenerateBinaryExpr((BinaryExpr*)expr);
        case Expr::CallExprClass:
            return GenerateCallExpr((CallExpr*)expr);
        default:
            cgm.GetCC().GetDiagnosticReporter().Error(Diagnostic::InternalError)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
    }
}

llvm::Value* VCL::CodeGenFunction::GenerateRValueExpr(Expr* expr) {
    llvm::Value* value = GenerateExpr(expr);
    if (expr->GetValueCategory() == Expr::RValue)
        return value;
    return builder.CreateLoad(cgm.GetCGT().ConvertType(expr->GetResultType()), value);
}

llvm::Value* VCL::CodeGenFunction::GenerateNumericLiteralExpr(NumericLiteralExpr* expr) {
    return cgm.GenerateConstantValue(expr->GetConstantValue());
}

llvm::Value* VCL::CodeGenFunction::GenerateDerefExpr(DerefExpr* expr) {
    llvm::Value* value = GenerateExpr(expr->GetExpr());
    return builder.CreateLoad(cgm.GetCGT().ConvertType(expr->GetResultType()), value);
}

llvm::Value* VCL::CodeGenFunction::GenerateDeclRefExpr(DeclRefExpr* expr) {
    ValueDecl* decl = expr->GetValueDecl();
    llvm::Value* value = GetDeclValue(decl);
    if (!value) {
        cgm.GetCC().GetDiagnosticReporter().Error(Diagnostic::InternalError)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return nullptr;
    }
    return value;
}

llvm::Value* VCL::CodeGenFunction::GenerateCastExpr(CastExpr* expr) {
    llvm::Value* exprValue = GenerateRValueExpr(expr->GetExpr());
    llvm::Type* dstType = cgm.GetCGT().ConvertType(expr->GetResultType());

    if (!exprValue || !dstType)
        return nullptr;

    switch (expr->GetCastKind()) {
        case CastExpr::FloatingCastExt: return builder.CreateFPExt(exprValue, dstType);
        case CastExpr::FloatingCastTrunc: return builder.CreateFPTrunc(exprValue, dstType);
        case CastExpr::FloatingToSigned: return builder.CreateFPToSI(exprValue, dstType);
        case CastExpr::FloatingToUnsigned: return builder.CreateFPToUI(exprValue, dstType);

        case CastExpr::SignedCastExt:
        case CastExpr::UnsignedCastExt:
        case CastExpr::SignedCastTrunc:
        case CastExpr::UnsignedCastTrunc: return builder.CreateZExtOrTrunc(exprValue, dstType);
        
        case CastExpr::SignedToFloating: return builder.CreateSIToFP(exprValue, dstType);
        case CastExpr::SignedToUnsigned: return builder.CreateIntCast(exprValue, dstType, false);
        case CastExpr::UnsignedToFloating: return builder.CreateUIToFP(exprValue, dstType);
        case CastExpr::UnsignedToSigned: return builder.CreateIntCast(exprValue, dstType, true);

        default:
            cgm.GetCC().GetDiagnosticReporter().Error(Diagnostic::InternalError)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
    }
}

llvm::Value* VCL::CodeGenFunction::GenerateSplatExpr(SplatExpr* expr) {
    llvm::Value* value = GenerateRValueExpr(expr->GetExpr());
    if (!value)
        return nullptr;
    return builder.CreateVectorSplat(cgm.GetTarget().GetVectorWidthInElement(), value);
}

llvm::Value* VCL::CodeGenFunction::GenerateBinaryExpr(BinaryExpr* expr) {
    Expr* lhsExpr = expr->GetLHS();
    Expr* rhsExpr = expr->GetRHS();

    switch (expr->GetOperatorKind()) {
        case BinaryOperator::Add:
            return DispatchBinaryArithmeticOp(lhsExpr, rhsExpr, llvm::Instruction::Add, llvm::Instruction::FAdd);
        case BinaryOperator::Sub:
            return DispatchBinaryArithmeticOp(lhsExpr, rhsExpr, llvm::Instruction::Sub, llvm::Instruction::FSub);
        case BinaryOperator::Mul:
            return DispatchBinaryArithmeticOp(lhsExpr, rhsExpr, llvm::Instruction::Mul, llvm::Instruction::FMul);
        case BinaryOperator::Div:
            return DispatchBinaryArithmeticOp(lhsExpr, rhsExpr, llvm::Instruction::SDiv, llvm::Instruction::UDiv, llvm::Instruction::FDiv);
        case BinaryOperator::Remainder:
            return DispatchBinaryArithmeticOp(lhsExpr, rhsExpr, llvm::Instruction::SRem, llvm::Instruction::URem, llvm::Instruction::FRem);
        case BinaryOperator::Assignment: {
            llvm::Value* lhsExprValue = GenerateExpr(lhsExpr);
            llvm::Value* rhsExprValue = GenerateRValueExpr(rhsExpr);
            if (!lhsExprValue || !rhsExprValue)
                return nullptr;
            return builder.CreateStore(rhsExprValue, lhsExprValue);
        }
        default:
            cgm.GetCC().GetDiagnosticReporter().Error(Diagnostic::InternalError)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .AddHint(DiagnosticHint{ expr->GetSourceRange() })
                .Report();
            return nullptr;
    }
}

llvm::Value* VCL::CodeGenFunction::DispatchBinaryArithmeticOp(Expr* lhs, Expr* rhs, llvm::Instruction::BinaryOps signedop, 
        llvm::Instruction::BinaryOps unsignedop, llvm::Instruction::BinaryOps floatop) {
    Type* type = lhs->GetResultType().GetType();
    if (type->GetTypeClass() == Type::TemplateSpecializationTypeClass)
        type = ((TemplateSpecializationType*)type)->GetInstantiatedType();
    if (type->GetTypeClass() == Type::VectorTypeClass)
        type = ((VectorType*)type)->GetElementType().GetType();
    if (type->GetTypeClass() != Type::BuiltinTypeClass) {
        cgm.GetCC().GetDiagnosticReporter().Error(Diagnostic::InternalError)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ lhs->GetSourceRange() })
            .Report();
        return nullptr;
    }

    BuiltinType::Kind kind = ((BuiltinType*)type)->GetKind();

    llvm::Value* lhsExprValue = GenerateRValueExpr(lhs);
    llvm::Value* rhsExprValue = GenerateRValueExpr(rhs);

    if (!lhsExprValue || !rhsExprValue)
        return nullptr;

    switch (kind) {
        case BuiltinType::UInt8:
        case BuiltinType::UInt16:
        case BuiltinType::UInt32:
        case BuiltinType::UInt64:
            return builder.CreateBinOp(unsignedop, lhsExprValue, rhsExprValue);
        case BuiltinType::Int8:
        case BuiltinType::Int16:
        case BuiltinType::Int32:
        case BuiltinType::Int64:
            return builder.CreateBinOp(signedop, lhsExprValue, rhsExprValue);
        case BuiltinType::Float32:
        case BuiltinType::Float64:
            return builder.CreateBinOp(floatop, lhsExprValue, rhsExprValue);
        default:
            cgm.GetCC().GetDiagnosticReporter().Error(Diagnostic::InternalError)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .AddHint(DiagnosticHint{ lhs->GetSourceRange() })
                .Report();
            return nullptr;
    }
}

llvm::Value* VCL::CodeGenFunction::DispatchBinaryArithmeticOp(Expr* lhs, Expr* rhs, llvm::Instruction::BinaryOps signedop, 
        llvm::Instruction::BinaryOps floatop) {
    return DispatchBinaryArithmeticOp(lhs, rhs, signedop, signedop, floatop);
}

llvm::Value* VCL::CodeGenFunction::GenerateCallExpr(CallExpr* expr) {
    llvm::Value* value = GetDeclValue(expr->GetFunctionDecl());
    if (!value) {
        cgm.GetCC().GetDiagnosticReporter().Error(Diagnostic::InternalError)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return nullptr;
    }
    llvm::SmallVector<llvm::Value*> argsValue{};
    for (auto arg : expr->GetArgs()) {
        llvm::Value* argValue = GenerateExpr(arg);
        if (!argValue)
            return nullptr;
        argsValue.push_back(argValue);
    }

    return builder.CreateCall(llvm::cast<llvm::Function>(value), argsValue);
}