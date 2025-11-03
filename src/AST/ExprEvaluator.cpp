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
            return Visit(initializer);
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
        case VCL::BuiltinType::UInt16: return CastFromTo<T, uint8_t>(context, value);
        case VCL::BuiltinType::UInt32: return CastFromTo<T, uint8_t>(context, value);
        case VCL::BuiltinType::UInt64: return CastFromTo<T, uint8_t>(context, value);
        default: return nullptr;
    }
}

VCL::ConstantValue* VCL::ExprEvaluator::VisitCastExpr(CastExpr* expr) {
    ConstantValue* value = Visit(expr->GetExpr());
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
        case BuiltinType::UInt16: return CastFrom<uint8_t>(context, expr->GetResultType(), scalar);
        case BuiltinType::UInt32: return CastFrom<uint8_t>(context, expr->GetResultType(), scalar);
        case BuiltinType::UInt64: return CastFrom<uint8_t>(context, expr->GetResultType(), scalar);
        default: return nullptr;
    }
}

VCL::ConstantValue* VCL::ExprEvaluator::VisitSplatExpr(SplatExpr* expr) {
    return Visit(expr->GetExpr());
}

VCL::ConstantValue* VCL::ExprEvaluator::VisitBinaryExpr(BinaryExpr* expr) {
    return nullptr;
}
