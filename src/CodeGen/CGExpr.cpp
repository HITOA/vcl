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
        case Expr::SubscriptExprClass:
            return GenerateSubscriptExpr((SubscriptExpr*)expr);
        case Expr::AggregateExprClass:
            return GenerateAggregateExpr((AggregateExpr*)expr);
        case Expr::NullExprClass:
            return GenerateNullExpr((NullExpr*)expr);
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
    llvm::Value* v = builder.CreateLoad(cgm.GetCGT().ConvertType(expr->GetResultType()), exprValue);
    return v;
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
            if (!lhsExprValue || !rhsExprValue) {
                return nullptr;
            }
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
    if (expr->GetFunctionDecl()->HasFunctionFlag(FunctionDecl::IsIntrinsic))
        return GenerateIntrinsicCallExpr(expr);

    llvm::Value* value = GetDeclValue(expr->GetFunctionDecl());
    if (!value) {
        cgm.GetDiagnosticReporter().Error(Diagnostic::InternalError)
            .AddHint(DiagnosticHint{ expr->GetSourceRange() })
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

llvm::Value* VCL::CodeGenFunction::GenerateIntrinsicCallExpr(CallExpr* expr) {
    llvm::SmallVector<llvm::Value*> argsValue{};
    for (auto arg : expr->GetArgs()) {
        llvm::Value* argValue = GenerateExpr(arg);
        if (!argValue)
            return nullptr;
        argsValue.push_back(argValue);
    }

    switch (expr->GetFunctionDecl()->GetIntrinsicID()) {
        // Unary Math
        case FunctionDecl::IntrinsicID::Sin: return builder.CreateUnaryIntrinsic(llvm::Intrinsic::sin, argsValue[0]);
        case FunctionDecl::IntrinsicID::Cos: return builder.CreateUnaryIntrinsic(llvm::Intrinsic::cos, argsValue[0]);
        case FunctionDecl::IntrinsicID::Tan: return builder.CreateUnaryIntrinsic(llvm::Intrinsic::tan, argsValue[0]);
        case FunctionDecl::IntrinsicID::Sinh: return builder.CreateUnaryIntrinsic(llvm::Intrinsic::sinh, argsValue[0]);
        case FunctionDecl::IntrinsicID::Cosh: return builder.CreateUnaryIntrinsic(llvm::Intrinsic::cosh, argsValue[0]);
        case FunctionDecl::IntrinsicID::Tanh: return builder.CreateUnaryIntrinsic(llvm::Intrinsic::tanh, argsValue[0]);
        case FunctionDecl::IntrinsicID::ASin: return builder.CreateUnaryIntrinsic(llvm::Intrinsic::asin, argsValue[0]);
        case FunctionDecl::IntrinsicID::ACos: return builder.CreateUnaryIntrinsic(llvm::Intrinsic::acos, argsValue[0]);
        case FunctionDecl::IntrinsicID::ATan: return builder.CreateUnaryIntrinsic(llvm::Intrinsic::atan, argsValue[0]);
        case FunctionDecl::IntrinsicID::Sqrt: return builder.CreateUnaryIntrinsic(llvm::Intrinsic::sqrt, argsValue[0]);
        case FunctionDecl::IntrinsicID::Log: return builder.CreateUnaryIntrinsic(llvm::Intrinsic::log, argsValue[0]);
        case FunctionDecl::IntrinsicID::Log2: return builder.CreateUnaryIntrinsic(llvm::Intrinsic::log2, argsValue[0]);
        case FunctionDecl::IntrinsicID::Log10: return builder.CreateUnaryIntrinsic(llvm::Intrinsic::log10, argsValue[0]);
        case FunctionDecl::IntrinsicID::Exp: return builder.CreateUnaryIntrinsic(llvm::Intrinsic::exp, argsValue[0]);
        case FunctionDecl::IntrinsicID::Exp2: return builder.CreateUnaryIntrinsic(llvm::Intrinsic::exp2, argsValue[0]);
        case FunctionDecl::IntrinsicID::Floor: return builder.CreateUnaryIntrinsic(llvm::Intrinsic::floor, argsValue[0]);
        case FunctionDecl::IntrinsicID::Ceil: return builder.CreateUnaryIntrinsic(llvm::Intrinsic::ceil, argsValue[0]);
        case FunctionDecl::IntrinsicID::Round: return builder.CreateUnaryIntrinsic(llvm::Intrinsic::round, argsValue[0]);
        case FunctionDecl::IntrinsicID::Abs: return builder.CreateUnaryIntrinsic(llvm::Intrinsic::abs, argsValue[0]);
        // Binary Math
        case FunctionDecl::IntrinsicID::ATan2: return builder.CreateBinaryIntrinsic(llvm::Intrinsic::atan2, argsValue[0], argsValue[1]);
        case FunctionDecl::IntrinsicID::Pow: return builder.CreateBinaryIntrinsic(llvm::Intrinsic::pow, argsValue[0], argsValue[1]);
        case FunctionDecl::IntrinsicID::Min: return builder.CreateBinaryIntrinsic(llvm::Intrinsic::minimum, argsValue[0], argsValue[1]);
        case FunctionDecl::IntrinsicID::Max: return builder.CreateBinaryIntrinsic(llvm::Intrinsic::maximum, argsValue[0], argsValue[1]);
        case FunctionDecl::IntrinsicID::FMod: return builder.CreateFRem(argsValue[0], argsValue[1]);
        // Ternary Math
        case FunctionDecl::IntrinsicID::Fma: return builder.CreateFMA(argsValue[0], argsValue[1], argsValue[2]);
        // Unpack & Pack
        case FunctionDecl::IntrinsicID::Unpack:
        case FunctionDecl::IntrinsicID::Pack: {
            llvm::Type* returnType = cgm.GetCGT().ConvertType(expr->GetFunctionDecl()->GetType()->GetReturnType());
            llvm::Align align{ (uint64_t)cgm.GetTarget().GetVectorWidthInByte() };
            llvm::Value* value = argsValue[0];
            if (!value->getType()->isPointerTy()) {
                llvm::AllocaInst* alloca = GenerateAllocaInst(value->getType(), "tmp");
                alloca->setAlignment(align);
                builder.CreateAlignedStore(value, alloca, align);
                value = alloca;
            }
            return builder.CreateAlignedLoad(returnType, value, align);
        }
        // Length
        case FunctionDecl::IntrinsicID::Length: {
            Type* type = Type::GetCanonicalType(expr->GetFunctionDecl()->GetType()->GetParamsType()[0].GetType());
            
            if (type->GetTypeClass() == Type::VectorTypeClass || type->GetTypeClass() == Type::LanesTypeClass) {
                return llvm::ConstantInt::get(cgm.GetLLVMContext(), llvm::APInt{ 64, cgm.GetTarget().GetVectorWidthInElement() });
            } else if (type->GetTypeClass() == Type::ArrayTypeClass) {
                uint64_t size = ((ArrayType*)type)->GetElementCount();
                return llvm::ConstantInt::get(cgm.GetLLVMContext(), llvm::APInt{ 64, size });
            } else if (type->GetTypeClass() == Type::SpanTypeClass) {
                llvm::StructType* spanType = cgm.GetCGT().ConvertSpanType(type);
                llvm::Value* v = builder.CreateLoad(spanType->getStructElementType(1), builder.CreateStructGEP(spanType, argsValue[0], 1));
                return v;
            } else {
                cgm.GetDiagnosticReporter().Error(Diagnostic::InternalError)
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .Report();
                return nullptr;
            }
        }
        // Select
        case FunctionDecl::IntrinsicID::Select: return builder.CreateSelect(argsValue[0], argsValue[1], argsValue[2]);
        default: 
            cgm.GetDiagnosticReporter().Error(Diagnostic::MissingImplementation)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
    }
}

llvm::Value* VCL::CodeGenFunction::GenerateFieldAccessExpr(FieldAccessExpr* expr) {
    llvm::Value* lhs = GenerateExpr(expr->GetExpr());
    if (!lhs)
        return nullptr;
    llvm::Type* recordType = cgm.GetCGT().ConvertRecordDeclType(expr->GetRecordType());
    return builder.CreateStructGEP(recordType, lhs, expr->GetFieldIndex());
}

llvm::Value* VCL::CodeGenFunction::GenerateSubscriptExpr(SubscriptExpr* expr) {
    llvm::Value* lhs = GenerateExpr(expr->GetExpr());
    llvm::Value* index = GenerateExpr(expr->GetIndex());

    if (expr->IsSpan()) {
        SpanType* spanType = (SpanType*)Type::GetCanonicalType(expr->GetExpr()->GetResultType().GetType());
        llvm::Type* spanTypeLLVM = cgm.GetCGT().ConvertSpanType(spanType);
        llvm::Type* type = cgm.GetCGT().ConvertType(spanType->GetElementType());
        lhs = builder.CreateLoad(spanTypeLLVM->getStructElementType(0), builder.CreateStructGEP(spanTypeLLVM, lhs, 0));
        llvm::Value* result = builder.CreateGEP(type, lhs, index);
        return result;
    } else {
        llvm::Type* type = cgm.GetCGT().ConvertType(Type::GetCanonicalType(expr->GetExpr()->GetResultType().GetType()));
        llvm::Value* zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(cgm.GetLLVMContext()), 0);
        llvm::Value* result = builder.CreateGEP(type, lhs, { zero, index });
        return result;
    }
}

llvm::Value* VCL::CodeGenFunction::GenerateAggregateExpr(AggregateExpr* expr) {
    llvm::Type* type = cgm.GetCGT().ConvertType(expr->GetResultType());
    llvm::Value* agg = llvm::PoisonValue::get(type);

    uint32_t i = 0;
    for (Expr* element : expr->GetElements()) {
        llvm::Value* value = GenerateExpr(element);
        if (!value)
            return nullptr;
        agg = builder.CreateInsertValue(agg, value, { i++ });
    }
    return agg;
}

llvm::Value* VCL::CodeGenFunction::GenerateNullExpr(NullExpr* expr) {
    llvm::Type* type = cgm.GetCGT().ConvertType(expr->GetResultType());
    return llvm::Constant::getNullValue(type);
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