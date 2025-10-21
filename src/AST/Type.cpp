#include <VCL/AST/Type.hpp>

#include <VCL/AST/DeclTemplate.hpp>
#include <VCL/AST/Template.hpp>


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