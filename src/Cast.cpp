#include "Cast.hpp"

#include <VCL/Debug.hpp>

#include "Type.hpp"
#include "Utility.hpp"


std::expected<VCL::CastResult, VCL::Error> VCL::ArithmeticImplicitCast(Handle<Value> lhs, Handle<Value> rhs) {
    Handle<Value> target = lhs;
    Handle<Value> casted = rhs;
    bool isLHSCasted = false;

    if (lhs->GetType() == rhs->GetType())
        return CastResult{ lhs, casted };

    if (!lhs->GetType().GetTypeInfo()->IsVector() && rhs->GetType().GetTypeInfo()->IsVector()) {
        casted = lhs;
        target = rhs;
        isLHSCasted = true;
    } else if (lhs->GetType().GetTypeInfo()->IsVector() == rhs->GetType().GetTypeInfo()->IsVector()) {
        TypeInfo::TypeName lhsScalarTypeName = GetScalarTypeName(lhs->GetType().GetTypeInfo()->type);
        TypeInfo::TypeName rhsScalarTypeName = GetScalarTypeName(rhs->GetType().GetTypeInfo()->type);
        
        if (lhsScalarTypeName != TypeInfo::TypeName::Double && rhsScalarTypeName == TypeInfo::TypeName::Double) {
            casted = lhs;
            target = rhs;
            isLHSCasted = true;
        } else if (lhsScalarTypeName != TypeInfo::TypeName::Float && lhsScalarTypeName != TypeInfo::TypeName::Double && 
                rhsScalarTypeName == TypeInfo::TypeName::Float) {
            casted = lhs;
            target = rhs;
            isLHSCasted = true;
        }
    }

    TypeInfo::TypeName targetScalarTypeName = GetScalarTypeName(target->GetType().GetTypeInfo()->type);
    TypeInfo::TypeName castedScalarTypeName = GetScalarTypeName(casted->GetType().GetTypeInfo()->type);

    if (targetScalarTypeName != TypeInfo::TypeName::Double && castedScalarTypeName == TypeInfo::TypeName::Double)
        return std::unexpected{ Error{ std::format("Implicit conversion from `{}` to `{}` may lead to loss of precision",
            ToString(casted->GetType().GetTypeInfo()), ToString(target->GetType().GetTypeInfo())) } };
    else if (targetScalarTypeName != TypeInfo::TypeName::Float && targetScalarTypeName != TypeInfo::TypeName::Double && 
        castedScalarTypeName == TypeInfo::TypeName::Float)
        return std::unexpected{ Error{ std::format("Implicit conversion from `{}` to `{}` may lead to loss of precision",
            ToString(casted->GetType().GetTypeInfo()), ToString(target->GetType().GetTypeInfo())) } };

    if (auto r = casted->Cast(target->GetType()); r.has_value())
        casted = *r;
    else
        return std::unexpected{ r.error() };

    if (isLHSCasted)
        return CastResult{ casted, rhs };
    else
        return CastResult{ lhs, casted };
}