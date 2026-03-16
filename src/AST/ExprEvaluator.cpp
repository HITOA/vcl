#include <VCL/AST/ExprEvaluator.hpp>


VCL::ConstantValue* VCL::ExprEvaluator::VisitNumericLiteralExpr(NumericLiteralExpr* expr) {
    return expr->GetValue();
}

VCL::ConstantValue* VCL::ExprEvaluator::VisitDeclRefExpr(DeclRefExpr* expr) {
    Decl* decl = expr->GetValueDecl();
    switch (decl->GetDeclClass()) {
        case Decl::VarDeclClass: {
            VarDecl* varDecl = (VarDecl*)decl;
            if (!varDecl->GetValueType().HasQualifier(Qualifier::Const))
                return nullptr;
            Expr* initializer = varDecl->GetInitializer();
            if (!initializer)
                return nullptr;
            return VisitExpr(initializer);
        }
        default: return nullptr;
    }
}

template<typename T, typename U>
VCL::ConstantScalar* CastFromTo(VCL::ASTContext& context, VCL::ConstantScalar* value) {
    U v = (U)(value->Get<T>());
    return context.AllocateNode<VCL::ConstantScalar>((U)v);
}

template<typename T>
VCL::ConstantScalar* CastFrom(VCL::ASTContext& context, VCL::QualType to, VCL::ConstantScalar* value) {
    VCL::Type* type = to.GetType();
    if (type->GetTypeClass() == VCL::Type::TemplateSpecializationTypeClass)
        type = ((VCL::TemplateSpecializationType*)type)->GetInstantiatedType();
    if (type->GetTypeClass() == VCL::Type::VectorTypeClass)
        type = ((VCL::VectorType*)type)->GetElementType().GetType();
    if (type->GetTypeClass() != VCL::Type::BuiltinTypeClass)
        return nullptr;
    switch (((VCL::BuiltinType*)type)->GetKind()) {
        case VCL::BuiltinType::Float32: return CastFromTo<T, float>(context, value);
        case VCL::BuiltinType::Float64: return CastFromTo<T, double>(context, value);
        case VCL::BuiltinType::Int8: return CastFromTo<T, int8_t>(context, value);
        case VCL::BuiltinType::Int16: return CastFromTo<T, int16_t>(context, value);
        case VCL::BuiltinType::Int32: return CastFromTo<T, int32_t>(context, value);
        case VCL::BuiltinType::Int64: return CastFromTo<T, int64_t>(context, value);
        case VCL::BuiltinType::UInt8: return CastFromTo<T, uint8_t>(context, value);
        case VCL::BuiltinType::UInt16: return CastFromTo<T, uint16_t>(context, value);
        case VCL::BuiltinType::UInt32: return CastFromTo<T, uint32_t>(context, value);
        case VCL::BuiltinType::UInt64: return CastFromTo<T, uint64_t>(context, value);
        default: return nullptr;
    }
}

VCL::ConstantValue* VCL::ExprEvaluator::VisitCastExpr(CastExpr* expr) {
    ConstantValue* value = VisitExpr(expr->GetExpr());
    if (!value)
        return nullptr;
    if (value->GetConstantValueClass() != ConstantValue::ConstantScalarClass)
        return nullptr;
    ConstantScalar* scalar = (ConstantScalar*)value;
    switch (scalar->GetKind()) {
        case BuiltinType::Float32: return CastFrom<float>(context, expr->GetResultType(), scalar);
        case BuiltinType::Float64: return CastFrom<double>(context, expr->GetResultType(), scalar);
        case BuiltinType::Int8: return CastFrom<int8_t>(context, expr->GetResultType(), scalar);
        case BuiltinType::Int16: return CastFrom<int16_t>(context, expr->GetResultType(), scalar);
        case BuiltinType::Int32: return CastFrom<int32_t>(context, expr->GetResultType(), scalar);
        case BuiltinType::Int64: return CastFrom<int64_t>(context, expr->GetResultType(), scalar);
        case BuiltinType::UInt8: return CastFrom<uint8_t>(context, expr->GetResultType(), scalar);
        case BuiltinType::UInt16: return CastFrom<uint16_t>(context, expr->GetResultType(), scalar);
        case BuiltinType::UInt32: return CastFrom<uint32_t>(context, expr->GetResultType(), scalar);
        case BuiltinType::UInt64: return CastFrom<uint64_t>(context, expr->GetResultType(), scalar);
        default: return nullptr;
    }
}

VCL::ConstantValue* VCL::ExprEvaluator::VisitSplatExpr(SplatExpr* expr) {
    return VisitExpr(expr->GetExpr());
}

template<typename T>
T BinaryOp(T a, T b, VCL::BinaryOperator::Kind op) {
    switch (op) {
        case VCL::BinaryOperator::Kind::Add:
            return a + b;
        case VCL::BinaryOperator::Kind::Sub:
            return a - b;
        case VCL::BinaryOperator::Kind::Mul:
            return a * b;
        case VCL::BinaryOperator::Kind::Div:
            return a / b;
        default:
            assert(false && "unsupported binary operator at compile time");
    }
}

VCL::ConstantValue* VCL::ExprEvaluator::VisitBinaryExpr(BinaryExpr* expr) {
    ConstantValue* lhs = VisitExpr(expr->GetLHS());
    ConstantValue* rhs = VisitExpr(expr->GetRHS());

    if (!lhs || !rhs)
        return nullptr;

    if (lhs->GetConstantValueClass() != ConstantValue::ConstantScalarClass || 
        rhs->GetConstantValueClass() != ConstantValue::ConstantScalarClass)
        return nullptr;
    
    ConstantScalar* lhsScalar = (ConstantScalar*)lhs;
    ConstantScalar* rhsScalar = (ConstantScalar*)rhs;

    BinaryOperator::Kind op = expr->GetOperatorKind();

    switch (lhsScalar->GetKind()) {
        case BuiltinType::Float32:
            return context.AllocateNode<ConstantScalar>(BinaryOp(lhsScalar->Get<float>(), rhsScalar->Get<float>(), op));
        case BuiltinType::Float64:
            return context.AllocateNode<ConstantScalar>(BinaryOp(lhsScalar->Get<double>(), rhsScalar->Get<double>(), op));
        case BuiltinType::Int8:
            return context.AllocateNode<ConstantScalar>(BinaryOp(lhsScalar->Get<int8_t>(), rhsScalar->Get<int8_t>(), op));
        case BuiltinType::Int16:
            return context.AllocateNode<ConstantScalar>(BinaryOp(lhsScalar->Get<int16_t>(), rhsScalar->Get<int16_t>(), op));
        case BuiltinType::Int32:
            return context.AllocateNode<ConstantScalar>(BinaryOp(lhsScalar->Get<int32_t>(), rhsScalar->Get<int32_t>(), op));
        case BuiltinType::Int64:
            return context.AllocateNode<ConstantScalar>(BinaryOp(lhsScalar->Get<int64_t>(), rhsScalar->Get<int64_t>(), op));
        case BuiltinType::UInt8:
            return context.AllocateNode<ConstantScalar>(BinaryOp(lhsScalar->Get<uint8_t>(), rhsScalar->Get<uint8_t>(), op));
        case BuiltinType::UInt16:
            return context.AllocateNode<ConstantScalar>(BinaryOp(lhsScalar->Get<uint16_t>(), rhsScalar->Get<uint16_t>(), op));
        case BuiltinType::UInt32:
            return context.AllocateNode<ConstantScalar>(BinaryOp(lhsScalar->Get<uint32_t>(), rhsScalar->Get<uint32_t>(), op));
        case BuiltinType::UInt64:
            return context.AllocateNode<ConstantScalar>(BinaryOp(lhsScalar->Get<uint64_t>(), rhsScalar->Get<uint64_t>(), op));
        default:
            return nullptr;
    }
}

template<typename T>
T UnaryOp(T value, VCL::UnaryOperator op) {
    switch (op) {
        case VCL::UnaryOperator::Minus:
            return -value;
        case VCL::UnaryOperator::Plus:
            return value;
        default:
            assert(false && "unsupported unary operator at compile time");
    }
}

VCL::ConstantValue* VCL::ExprEvaluator::VisitUnaryExpr(UnaryExpr* expr) {
    ConstantValue* value = VisitExpr(expr->GetExpr());
    if (value->GetConstantValueClass() != ConstantValue::ConstantScalarClass)
        return nullptr;

    UnaryOperator op = expr->GetOperator();
    ConstantScalar* scalar = (ConstantScalar*)value;
    
    switch (scalar->GetKind()) {
        case BuiltinType::Float32:
            return context.AllocateNode<ConstantScalar>(UnaryOp(scalar->Get<float>(), op));
        case BuiltinType::Float64:
            return context.AllocateNode<ConstantScalar>(UnaryOp(scalar->Get<double>(), op));
        case BuiltinType::Int8:
            return context.AllocateNode<ConstantScalar>(UnaryOp(scalar->Get<int8_t>(), op));
        case BuiltinType::Int16:
            return context.AllocateNode<ConstantScalar>(UnaryOp(scalar->Get<int16_t>(), op));
        case BuiltinType::Int32:
            return context.AllocateNode<ConstantScalar>(UnaryOp(scalar->Get<int32_t>(), op));
        case BuiltinType::Int64:
            return context.AllocateNode<ConstantScalar>(UnaryOp(scalar->Get<int64_t>(), op));
        case BuiltinType::UInt8:
            return context.AllocateNode<ConstantScalar>(UnaryOp(scalar->Get<uint8_t>(), op));
        case BuiltinType::UInt16:
            return context.AllocateNode<ConstantScalar>(UnaryOp(scalar->Get<uint16_t>(), op));
        case BuiltinType::UInt32:
            return context.AllocateNode<ConstantScalar>(UnaryOp(scalar->Get<uint32_t>(), op));
        case BuiltinType::UInt64:
            return context.AllocateNode<ConstantScalar>(UnaryOp(scalar->Get<uint64_t>(), op));
        default:
            return nullptr;
    }
}

VCL::ConstantValue* VCL::ExprEvaluator::VisitAggregateExpr(AggregateExpr* expr) {
    llvm::SmallVector<ConstantValue*> values{};
    values.reserve(expr->GetElementCount());

    for (Expr* element : expr->GetElements()) {
        ConstantValue* value = VisitExpr(element);
        if (!value)
            return nullptr;
        values.push_back(value);
    }

    return context.AllocateNode<ConstantAggregate>(values, expr->GetResultType());
}

VCL::ConstantValue* VCL::ExprEvaluator::VisitNullExpr(NullExpr* expr) {
    return context.AllocateNode<ConstantNull>(expr->GetResultType());
}

VCL::ConstantValue* VCL::ExprEvaluator::VisitLoadExpr(LoadExpr* expr) {
    return VisitExpr(expr->GetExpr());
}

VCL::ConstantValue* VCL::ExprEvaluator::VisitCallExpr(CallExpr* expr) {
    return nullptr;
}

VCL::ConstantValue* VCL::ExprEvaluator::VisitDependentCallExpr(DependentCallExpr* expr) {
    return nullptr;
}

VCL::ConstantValue* VCL::ExprEvaluator::VisitFieldAccessExpr(FieldAccessExpr* expr) {
    return nullptr;
}

VCL::ConstantValue* VCL::ExprEvaluator::VisitDependentFieldAccessExpr(DependentFieldAccessExpr* expr) {
    return nullptr;
}

VCL::ConstantValue* VCL::ExprEvaluator::VisitSubscriptExpr(SubscriptExpr* expr) {
    return nullptr;
}
