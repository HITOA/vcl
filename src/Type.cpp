#include "Type.hpp"

#include "Utility.hpp"


VCL::Type::Type(TypeInfo typeInfo, llvm::Type* type, ModuleContext* context) :
    typeInfo{ typeInfo }, type{ type }, context{ context } {}

VCL::TypeInfo VCL::Type::GetTypeInfo() const {
    return typeInfo;
}

llvm::Type* VCL::Type::GetLLVMType() const {
    return type;
}

bool VCL::Type::operator==(Type& rhs) const {
    return this->typeInfo.type == rhs.typeInfo.type && 
        this->typeInfo.compositeType == rhs.typeInfo.compositeType;
}

bool VCL::Type::operator!=(Type& rhs) const {
    return !(*this != rhs);
}

std::expected<VCL::Type, VCL::Error> VCL::Type::Create(TypeInfo typeInfo, ModuleContext* context) {
    llvm::Type* type;

    switch (typeInfo.type)
    {
    case TypeInfo::TypeName::FLOAT:
        type = llvm::Type::getFloatTy(context->GetContext());
        break;
    case TypeInfo::TypeName::BOOLEAN:
        type = llvm::Type::getInt1Ty(context->GetContext());
        break;
    case TypeInfo::TypeName::INT:
        type = llvm::Type::getInt32Ty(context->GetContext());
        break;
    case TypeInfo::TypeName::VOID:
        type = llvm::Type::getVoidTy(context->GetContext());
        break;
    case TypeInfo::TypeName::VFLOAT:
        type = llvm::FixedVectorType::get(llvm::Type::getFloatTy(context->GetContext()), GetMaxVectorElementWidth(sizeof(float)));
        break;
    default:
        return std::unexpected(Error{ "Invalid typename while parsing type" });
    }

    return Type{ typeInfo, type, context };
}

std::expected<VCL::Type, VCL::Error> VCL::Type::CreateFromLLVMType(llvm::Type* type, ModuleContext* context) {
    llvm::Type* scalarType = type;
    if (type->isVectorTy())
        scalarType = type->getScalarType();

    TypeInfo typeInfo;
    
    if (type->isFunctionTy())
        typeInfo.type = TypeInfo::TypeName::CALLABLE;

    if (scalarType->isFloatTy())
        typeInfo.type = TypeInfo::TypeName::FLOAT;
    else if (scalarType->isIntegerTy()) {
        uint32_t width = scalarType->getIntegerBitWidth();
        if (width == 1)
            typeInfo.type = TypeInfo::TypeName::BOOLEAN;
        else if (width == 32)
            typeInfo.type = TypeInfo::TypeName::INT;
        else
            return std::unexpected(Error{ "Unsupported integer bit width" });
    } else if (scalarType->isVoidTy())
        typeInfo.type = TypeInfo::TypeName::VOID;
    else
        return std::unexpected(Error{ "Unsupported LLVM type" });

    if (type->isVectorTy())
        typeInfo.type = (TypeInfo::TypeName)((uint32_t)typeInfo.type + 
            (uint32_t)TypeInfo::TypeName::VFLOAT - (uint32_t)TypeInfo::TypeName::FLOAT);
    
    return Type{ typeInfo, type, context };
}