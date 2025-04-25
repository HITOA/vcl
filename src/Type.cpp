#include "Type.hpp"

#include "ModuleContext.hpp"
#include "Utility.hpp"
#include "NativeTarget.hpp"


VCL::Type::Type() :
    typeInfo{}, type{ nullptr }, context{ nullptr } {}

VCL::Type::Type(TypeInfo typeInfo, llvm::Type* type, ModuleContext* context) :
    typeInfo{ typeInfo }, type{ type }, context{ context } {}

VCL::TypeInfo VCL::Type::GetTypeInfo() const {
    return typeInfo;
}

llvm::Type* VCL::Type::GetLLVMType() const {
    return type;
}

bool VCL::Type::operator==(Type& rhs) const {
    return this->typeInfo.type == rhs.typeInfo.type && this->typeInfo.name == rhs.typeInfo.name;
}

bool VCL::Type::operator!=(Type& rhs) const {
    return !(*this != rhs);
}

std::expected<VCL::Type, VCL::Error> VCL::Type::Create(TypeInfo typeInfo, ModuleContext* context) {
    llvm::Type* type;
    switch (typeInfo.type)
    {
    case TypeInfo::TypeName::Callable:
        type = nullptr;
        break;
    case TypeInfo::TypeName::Float:
        type = llvm::Type::getFloatTy(*context->GetTSContext().getContext());
        break;
    case TypeInfo::TypeName::Bool:
        type = llvm::Type::getInt1Ty(*context->GetTSContext().getContext());
        break;
    case TypeInfo::TypeName::Int:
        type = llvm::Type::getInt32Ty(*context->GetTSContext().getContext());
        break;
    case TypeInfo::TypeName::Void:
        type = llvm::Type::getVoidTy(*context->GetTSContext().getContext());
        break;
    case TypeInfo::TypeName::VectorFloat:
        type = llvm::FixedVectorType::get(llvm::Type::getFloatTy(*context->GetTSContext().getContext()), 
            NativeTarget::GetInstance()->GetMaxVectorElementWidth());
        break;
    case TypeInfo::TypeName::VectorBool:
        type = llvm::FixedVectorType::get(llvm::Type::getInt1Ty(*context->GetTSContext().getContext()), 
            NativeTarget::GetInstance()->GetMaxVectorElementWidth());
        break;
    case TypeInfo::TypeName::VectorInt:
        type = llvm::FixedVectorType::get(llvm::Type::getInt32Ty(*context->GetTSContext().getContext()), 
            NativeTarget::GetInstance()->GetMaxVectorElementWidth());
        break;
    case TypeInfo::TypeName::Custom:
        if (auto t = context->GetScopeManager().GetNamedType(typeInfo.name); t.has_value())
            type = (*t)->type;
        else
            return std::unexpected(t.error());
        break;
    default:
        return std::unexpected(Error{ "Invalid typename while parsing type" });
    }

    return Type{ typeInfo, type, context };
}

std::expected<VCL::Type, VCL::Error> VCL::Type::CreateFromLLVMType(llvm::Type* type, ModuleContext* context) {
    llvm::Type* trueType = type;
    if (type->isVectorTy())
        trueType = type->getScalarType();

    TypeInfo typeInfo;

    if (trueType->isStructTy()) {
        typeInfo.type = TypeInfo::TypeName::Custom;
        typeInfo.name = trueType->getStructName();
        return Type{ typeInfo, type, context };
    }
    
    if (trueType->isFunctionTy()) {
        typeInfo.type = TypeInfo::TypeName::Callable;
        return Type{ typeInfo, type, context };
    }

    if (trueType->isFloatTy())
        typeInfo.type = TypeInfo::TypeName::Float;
    else if (trueType->isIntegerTy()) {
        uint32_t width = trueType->getIntegerBitWidth();
        if (width == 1)
            typeInfo.type = TypeInfo::TypeName::Bool;
        else if (width == 32)
            typeInfo.type = TypeInfo::TypeName::Int;
        else
            return std::unexpected(Error{ "Unsupported integer bit width" });
    } else if (trueType->isVoidTy())
        typeInfo.type = TypeInfo::TypeName::Void;
    else
        return std::unexpected(Error{ "Unsupported LLVM type" });

    if (type->isVectorTy())
        typeInfo.type = (TypeInfo::TypeName)((uint32_t)typeInfo.type + 
            (uint32_t)TypeInfo::TypeName::VectorFloat - (uint32_t)TypeInfo::TypeName::Float);
    
    return Type{ typeInfo, type, context };
}