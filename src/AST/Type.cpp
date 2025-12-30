#include <VCL/AST/Type.hpp>

#include <VCL/AST/DeclTemplate.hpp>
#include <VCL/AST/Template.hpp>


bool VCL::Type::IsTypeNumeric(Type* type) {
    switch (type->GetTypeClass()) {
        case Type::BuiltinTypeClass: {
            BuiltinType::Kind kind = ((BuiltinType*)type)->GetKind();
            return kind != BuiltinType::Void;
        }
        case Type::VectorTypeClass: {
            Type* ofType = ((VectorType*)type)->GetElementType().GetType();
            return IsTypeNumeric(ofType);
        }
        case Type::ReferenceTypeClass: {
            Type* ofType = ((ReferenceType*)type)->GetType().GetType();
            return IsTypeNumeric(ofType);
        }
        case Type::TemplateSpecializationTypeClass: {
            Type* instantiatedType = ((TemplateSpecializationType*)type)->GetInstantiatedType();
            if (!instantiatedType)
                return false;
            return IsTypeNumeric(instantiatedType);
        }
        default:
            return false;
    }
}

bool VCL::Type::IsTypeIntegral(Type* type) {
    switch (type->GetTypeClass()) {
        case Type::BuiltinTypeClass: {
            BuiltinType::Kind kind = ((BuiltinType*)type)->GetKind();
            BuiltinType::Category category = BuiltinType::GetKindCategory(kind);
            return category == BuiltinType::Category::SignedKind || category == BuiltinType::Category::UnsignedKind;
        }
        case Type::VectorTypeClass: {
            Type* ofType = ((VectorType*)type)->GetElementType().GetType();
            return IsTypeIntegral(ofType);
        }
        case Type::ReferenceTypeClass: {
            Type* ofType = ((ReferenceType*)type)->GetType().GetType();
            return IsTypeIntegral(ofType);
        }
        case Type::TemplateSpecializationTypeClass: {
            Type* instantiatedType = ((TemplateSpecializationType*)type)->GetInstantiatedType();
            if (!instantiatedType)
                return false;
            return IsTypeIntegral(instantiatedType);
        }
        default:
            return false;
    }
}

VCL::Type* VCL::Type::GetCanonicalType(Type* type) {
    switch (type->GetTypeClass()) {
        case Type::ReferenceTypeClass:
            return GetCanonicalType(((ReferenceType*)type)->GetType().GetType());
        case Type::TemplateSpecializationTypeClass:
            return GetCanonicalType(((TemplateSpecializationType*)type)->GetInstantiatedType());
        default:
            return type;
    }
}

uint32_t VCL::BuiltinType::GetKindBitWidth(Kind kind) {
    switch (kind) {
        case Float32: return 32;
        case Float64: return 64;
        case Int8: return 8;
        case Int16: return 16;
        case Int32: return 32;
        case Int64: return 64;
        case Bool: return 1;
        case UInt8: return 8;
        case UInt16: return 16;
        case UInt32: return 32;
        case UInt64: return 64;
        default: return 0;
    }
}

VCL::BuiltinType::Category VCL::BuiltinType::GetKindCategory(Kind kind) {
    switch (kind) {
        case Float32:
        case Float64: return FloatingPointKind;
        case Int8:
        case Int16:
        case Int32:
        case Int64: return SignedKind;
        case Bool:
        case UInt8:
        case UInt16:
        case UInt32:
        case UInt64: return UnsignedKind;
        default: return VoidKind;
    }
}

VCL::RecordType::RecordType(RecordDecl* decl) : decl{ decl }, Type{ Type::RecordTypeClass } {
    bool isDependent = false;
    for (auto it = decl->Begin(); it != decl->End(); ++it) {
        if (it->GetDeclClass() != Decl::FieldDeclClass)
            continue;
        FieldDecl* fieldDecl = (FieldDecl*)it.Get();
        isDependent |= fieldDecl->GetType().GetType()->IsDependent();
    }
    SetDependent(isDependent);
}

VCL::TemplateSpecializationType::TemplateSpecializationType(TemplateDecl* decl, TemplateArgumentList* args)
        : decl{ decl }, args{ args }, instantiatedType{ nullptr }, Type{ Type::TemplateSpecializationTypeClass } {
    SetDependent(args->IsDependent());
}

void VCL::TemplateSpecializationType::Profile(llvm::FoldingSetNodeID& id, TemplateDecl* decl, TemplateArgumentList* args) {
    id.AddPointer(decl);
    for (auto& arg : args->GetArgs()) {
        switch (arg.GetKind()) {
            case TemplateArgument::Type:
                id.AddInteger((uintptr_t)arg.GetType().GetAsOpaquePtr());
                break;
            case TemplateArgument::Integral:
                id.AddInteger(arg.GetIntegral().Get<uint64_t>());
                break;
            case TemplateArgument::Expression:
                id.AddPointer(arg.GetExpr());
                break;
            default:
                id.AddPointer(&arg);
                break;
        }
    }
}