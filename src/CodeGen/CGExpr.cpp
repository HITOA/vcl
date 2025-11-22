#include <VCL/CodeGen/CodeGenFunction.hpp>

#include <VCL/CodeGen/CodeGenModule.hpp>



llvm::Value* VCL::CodeGenFunction::GenerateExpr(Expr* expr) {
    switch (expr->GetExprClass()) {
        case Expr::NumericLiteralExprClass:
            return GenerateNumericLiteralExpr((NumericLiteralExpr*)expr);
        case Expr::LoadExprClass:
            return GenerateLoadExpr((LoadExpr*)expr);
        case Expr::DeclRefExprClass:
            return GenerateDeclRefExpr((DeclRefExpr*)expr);
        case Expr::CastExprClass:
            return GenerateCastExpr((CastExpr*)expr);
        case Expr::SplatExprClass:
            return GenerateSplatExpr((SplatExpr*)expr);
        case Expr::BinaryExprClass:
            return GenerateBinaryExpr((BinaryExpr*)expr);
        case Expr::UnaryExprClass:
            return GenerateUnaryExpr((UnaryExpr*)expr);
        case Expr::CallExprClass:
            return GenerateCallExpr((CallExpr*)expr);
        case Expr::FieldAccessExprClass:
            return GenerateFieldAccessExpr((FieldAccessExpr*)expr);
        default:
            cgm.GetDiagnosticReporter().Error(Diagnostic::InternalError)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
    }
}

llvm::Value* VCL::CodeGenFunction::GenerateNumericLiteralExpr(NumericLiteralExpr* expr) {
    return cgm.GenerateConstantValue(expr->GetConstantValue());
}

llvm::Value* VCL::CodeGenFunction::GenerateLoadExpr(LoadExpr* expr) {
    llvm::Value* exprValue = GenerateExpr(expr->GetExpr());
    if (!exprValue)
        return nullptr;
    return builder.CreateLoad(cgm.GetCGT().ConvertType(expr->GetResultType()), exprValue);
}

llvm::Value* VCL::CodeGenFunction::GenerateDeclRefExpr(DeclRefExpr* expr) {
    ValueDecl* decl = expr->GetValueDecl();
    llvm::Value* value = GetDeclValue(decl);
    if (!value) {
        cgm.GetDiagnosticReporter().Error(Diagnostic::InternalError)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return nullptr;
    }
    return value;
}

llvm::Value* VCL::CodeGenFunction::GenerateCastExpr(CastExpr* expr) {
    llvm::Value* exprValue = GenerateExpr(expr->GetExpr());
    llvm::Type* dstType = cgm.GetCGT().ConvertType(expr->GetResultType());

    if (!exprValue || !dstType)
        return nullptr;

    switch (expr->GetCastKind()) {
        case CastExpr::FloatingCastExt: return builder.CreateFPExt(exprValue, dstType);
        case CastExpr::FloatingCastTrunc: return builder.CreateFPTrunc(exprValue, dstType);
        case CastExpr::FloatingToSigned: return builder.CreateFPToSI(exprValue, dstType);
        case CastExpr::FloatingToUnsigned: return builder.CreateFPToUI(exprValue, dstType);

        case CastExpr::SignedCastExt:
        case CastExpr::SignedCastTrunc: return builder.CreateSExtOrTrunc(exprValue, dstType);
        case CastExpr::UnsignedCastExt:
        case CastExpr::UnsignedCastTrunc: return builder.CreateZExtOrTrunc(exprValue, dstType);
        
        case CastExpr::SignedToFloating: return builder.CreateSIToFP(exprValue, dstType);
        case CastExpr::SignedToUnsigned: return builder.CreateIntCast(exprValue, dstType, true);
        case CastExpr::UnsignedToFloating: return builder.CreateUIToFP(exprValue, dstType);
        case CastExpr::UnsignedToSigned: return builder.CreateIntCast(exprValue, dstType, false);

        default:
            cgm.GetDiagnosticReporter().Error(Diagnostic::InternalError)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
    }
}

llvm::Value* VCL::CodeGenFunction::GenerateSplatExpr(SplatExpr* expr) {
    llvm::Value* value = GenerateExpr(expr->GetExpr());
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
        case BinaryOperator::Greater:
            return DispatchBinaryComparisonOp(lhsExpr, rhsExpr, llvm::CmpInst::ICMP_SGT, llvm::CmpInst::ICMP_UGT, llvm::CmpInst::FCMP_OGT);
        case BinaryOperator::Lesser:
            return DispatchBinaryComparisonOp(lhsExpr, rhsExpr, llvm::CmpInst::ICMP_SLT, llvm::CmpInst::ICMP_ULT, llvm::CmpInst::FCMP_OLT);
        case BinaryOperator::GreaterEqual:
            return DispatchBinaryComparisonOp(lhsExpr, rhsExpr, llvm::CmpInst::ICMP_SGE, llvm::CmpInst::ICMP_UGE, llvm::CmpInst::FCMP_OGE);
        case BinaryOperator::LesserEqual:
            return DispatchBinaryComparisonOp(lhsExpr, rhsExpr, llvm::CmpInst::ICMP_SLE, llvm::CmpInst::ICMP_ULE, llvm::CmpInst::FCMP_OLE);
        case BinaryOperator::Equal:
            return DispatchBinaryComparisonOp(lhsExpr, rhsExpr, llvm::CmpInst::ICMP_EQ, llvm::CmpInst::FCMP_OEQ);
        case BinaryOperator::NotEqual:
            return DispatchBinaryComparisonOp(lhsExpr, rhsExpr, llvm::CmpInst::ICMP_NE, llvm::CmpInst::FCMP_ONE);
        case BinaryOperator::LogicalAnd: {
            llvm::Value* lhsExprValue = GenerateExpr(lhsExpr);
            llvm::Value* rhsExprValue = GenerateExpr(rhsExpr);

            if (!lhsExprValue || !rhsExprValue)
                return nullptr;
            return builder.CreateLogicalAnd(lhsExprValue, rhsExprValue);
        }
        case BinaryOperator::LogicalOr: {
            llvm::Value* lhsExprValue = GenerateExpr(lhsExpr);
            llvm::Value* rhsExprValue = GenerateExpr(rhsExpr);

            if (!lhsExprValue || !rhsExprValue)
                return nullptr;
            return builder.CreateLogicalOr(lhsExprValue, rhsExprValue);
        }
        case BinaryOperator::BitwiseAnd:
            return DispatchBinaryArithmeticOp(lhsExpr, rhsExpr, llvm::Instruction::And, llvm::Instruction::And);
        case BinaryOperator::BitwiseXor:
            return DispatchBinaryArithmeticOp(lhsExpr, rhsExpr, llvm::Instruction::Xor, llvm::Instruction::Xor);
        case BinaryOperator::BitwiseOr:
            return DispatchBinaryArithmeticOp(lhsExpr, rhsExpr, llvm::Instruction::Or, llvm::Instruction::Or);
        case BinaryOperator::LeftShift:
            return DispatchBinaryArithmeticOp(lhsExpr, rhsExpr, llvm::Instruction::Shl, llvm::Instruction::Shl);
        case BinaryOperator::RightShift:
            return DispatchBinaryArithmeticOp(lhsExpr, rhsExpr, llvm::Instruction::AShr, llvm::Instruction::AShr);
        case BinaryOperator::Assignment: {
            llvm::Value* lhsExprValue = GenerateExpr(lhsExpr);
            llvm::Value* rhsExprValue = GenerateExpr(rhsExpr);
            if (!lhsExprValue || !rhsExprValue)
                return nullptr;
            return builder.CreateStore(rhsExprValue, lhsExprValue);
        }
        default:
            cgm.GetDiagnosticReporter().Error(Diagnostic::InternalError)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .AddHint(DiagnosticHint{ expr->GetSourceRange() })
                .Report();
            return nullptr;
    }
}

llvm::Value* VCL::CodeGenFunction::GenerateUnaryExpr(UnaryExpr* expr) {
    switch (expr->GetOperator()) {
        case UnaryOperator::PrefixIncrement: {
            llvm::Value* exprValue = GenerateExpr(expr->GetExpr());
            llvm::Value* loadedExprValue = builder.CreateLoad(cgm.GetCGT().ConvertType(expr->GetResultType()), exprValue);
            llvm::Value* result = nullptr;
            if (loadedExprValue->getType()->isFloatingPointTy())
                result = builder.CreateFAdd(loadedExprValue, llvm::ConstantFP::get(loadedExprValue->getType(), 1.0));
            else
                result = builder.CreateAdd(loadedExprValue, llvm::ConstantInt::get(loadedExprValue->getType(), 1));
            builder.CreateStore(result, exprValue);
            return result;
        }
        case UnaryOperator::PrefixDecrement: {
            llvm::Value* exprValue = GenerateExpr(expr->GetExpr());
            llvm::Value* loadedExprValue = builder.CreateLoad(cgm.GetCGT().ConvertType(expr->GetResultType()), exprValue);
            llvm::Value* result = nullptr;
            if (loadedExprValue->getType()->isFloatingPointTy())
                result = builder.CreateFSub(loadedExprValue, llvm::ConstantFP::get(loadedExprValue->getType(), 1.0));
            else
                result = builder.CreateSub(loadedExprValue, llvm::ConstantInt::get(loadedExprValue->getType(), 1));
            builder.CreateStore(result, exprValue);
            return result;
        }
        case UnaryOperator::PostfixIncrement: {
            llvm::Value* exprValue = GenerateExpr(expr->GetExpr());
            llvm::Value* loadedExprValue = builder.CreateLoad(cgm.GetCGT().ConvertType(expr->GetResultType()), exprValue);
            llvm::Value* result = nullptr;
            if (loadedExprValue->getType()->isFloatingPointTy())
                result = builder.CreateFAdd(loadedExprValue, llvm::ConstantFP::get(loadedExprValue->getType(), 1.0));
            else
                result = builder.CreateAdd(loadedExprValue, llvm::ConstantInt::get(loadedExprValue->getType(), 1));
            builder.CreateStore(result, exprValue);
            return loadedExprValue;
        }
        case UnaryOperator::PostfixDecrement: {
            llvm::Value* exprValue = GenerateExpr(expr->GetExpr());
            llvm::Value* loadedExprValue = builder.CreateLoad(cgm.GetCGT().ConvertType(expr->GetResultType()), exprValue);
            llvm::Value* result = nullptr;
            if (loadedExprValue->getType()->isFloatingPointTy())
                result = builder.CreateFSub(loadedExprValue, llvm::ConstantFP::get(loadedExprValue->getType(), 1.0));
            else
                result = builder.CreateSub(loadedExprValue, llvm::ConstantInt::get(loadedExprValue->getType(), 1));
            builder.CreateStore(result, exprValue);
            return loadedExprValue;
        }
        case UnaryOperator::Plus: {
            llvm::Value* exprValue = GenerateExpr(expr->GetExpr());
            return exprValue;
        }
        case UnaryOperator::Minus: {
            llvm::Value* exprValue = GenerateExpr(expr->GetExpr());
            llvm::Value* result = nullptr;
            if (exprValue->getType()->isFloatingPointTy())
                result = builder.CreateFNeg(exprValue);
            else
                result = builder.CreateNeg(exprValue);
            return result;
        }
        case UnaryOperator::LogicalNot: {
            llvm::Value* exprValue = GenerateExpr(expr->GetExpr());
            return builder.CreateNot(exprValue);
        }
        case UnaryOperator::BitwiseNot: {
            llvm::Value* exprValue = GenerateExpr(expr->GetExpr());
            return builder.CreateNot(exprValue);
        }
        default:
            cgm.GetDiagnosticReporter().Error(Diagnostic::InternalError)
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
        cgm.GetDiagnosticReporter().Error(Diagnostic::InternalError)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ lhs->GetSourceRange() })
            .Report();
        return nullptr;
    }

    BuiltinType::Kind kind = ((BuiltinType*)type)->GetKind();

    llvm::Value* lhsExprValue = GenerateExpr(lhs);
    llvm::Value* rhsExprValue = GenerateExpr(rhs);

    if (!lhsExprValue || !rhsExprValue)
        return nullptr;
    
    switch (kind) {
        case BuiltinType::Bool:
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
            cgm.GetDiagnosticReporter().Error(Diagnostic::InternalError)
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
        cgm.GetDiagnosticReporter().Error(Diagnostic::InternalError)
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

llvm::Value* VCL::CodeGenFunction::GenerateFieldAccessExpr(FieldAccessExpr* expr) {
    llvm::Value* lhs = GenerateExpr(expr->GetExpr());
    if (!lhs)
        return nullptr;
    llvm::Type* recordType = cgm.GetCGT().ConvertRecordDeclType(expr->GetRecordType());
    return builder.CreateStructGEP(recordType, lhs, expr->GetFieldIndex());
}

llvm::Value* VCL::CodeGenFunction::DispatchBinaryComparisonOp(Expr* lhs, Expr* rhs, llvm::CmpInst::Predicate signedPredicate,
        llvm::CmpInst::Predicate unsignedPredicate, llvm::CmpInst::Predicate floatPredicate) {
    Type* type = lhs->GetResultType().GetType();
    if (type->GetTypeClass() == Type::TemplateSpecializationTypeClass)
        type = ((TemplateSpecializationType*)type)->GetInstantiatedType();
    if (type->GetTypeClass() == Type::VectorTypeClass)
        type = ((VectorType*)type)->GetElementType().GetType();
    if (type->GetTypeClass() != Type::BuiltinTypeClass) {
        cgm.GetDiagnosticReporter().Error(Diagnostic::InternalError)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ lhs->GetSourceRange() })
            .Report();
        return nullptr;
    }

    BuiltinType::Kind kind = ((BuiltinType*)type)->GetKind();

    llvm::Value* lhsExprValue = GenerateExpr(lhs);
    llvm::Value* rhsExprValue = GenerateExpr(rhs);

    if (!lhsExprValue || !rhsExprValue)
        return nullptr;
    
    switch (kind) {
        case BuiltinType::Bool:
        case BuiltinType::UInt8:
        case BuiltinType::UInt16:
        case BuiltinType::UInt32:
        case BuiltinType::UInt64:
            return builder.CreateCmp(unsignedPredicate, lhsExprValue, rhsExprValue);
        case BuiltinType::Int8:
        case BuiltinType::Int16:
        case BuiltinType::Int32:
        case BuiltinType::Int64:
            return builder.CreateCmp(signedPredicate, lhsExprValue, rhsExprValue);
        case BuiltinType::Float32:
        case BuiltinType::Float64:
            return builder.CreateFCmp(floatPredicate, lhsExprValue, rhsExprValue);
        default:
            cgm.GetDiagnosticReporter().Error(Diagnostic::InternalError)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .AddHint(DiagnosticHint{ lhs->GetSourceRange() })
                .Report();
            return nullptr;
    }
}

llvm::Value* VCL::CodeGenFunction::DispatchBinaryComparisonOp(Expr* lhs, Expr* rhs, llvm::CmpInst::Predicate signedPredicate,
        llvm::CmpInst::Predicate floatPredicate) {
    return DispatchBinaryComparisonOp(lhs, rhs, signedPredicate, signedPredicate, floatPredicate);
}