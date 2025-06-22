#include "Type.hpp"

#include <VCL/Debug.hpp>
#include <VCL/NativeTarget.hpp>

#include "ModuleContext.hpp"
#include "Utility.hpp"

#include <iostream>


VCL::Type::Type() :
    typeInfo{}, type{ nullptr }, diType{ nullptr }, context{ nullptr }, isPointer{ false } {}

VCL::Type::Type(std::shared_ptr<TypeInfo> typeInfo, llvm::Type* type, llvm::DIType* diType, ModuleContext* context, bool isPointer) :
    typeInfo{ typeInfo }, type{ type }, diType{ diType }, context{ context }, isPointer{ false } {}

std::shared_ptr<VCL::TypeInfo> VCL::Type::GetTypeInfo() const {
    return typeInfo;
}

bool VCL::Type::IsPointerType() const {
    return isPointer;
}

void VCL::Type::SetPointer() {
    isPointer = true;
}

llvm::Type* VCL::Type::GetLLVMType() const {
    return type;
}

llvm::DIType* VCL::Type::GetDIType() const {
    return diType;
}

bool VCL::Type::operator==(Type& rhs) const {
    return this->typeInfo->type == rhs.typeInfo->type && this->typeInfo->name == rhs.typeInfo->name;
}

bool VCL::Type::operator!=(Type& rhs) const {
    return !(*this != rhs);
}

std::expected<VCL::Type, VCL::Error> VCL::Type::Create(std::shared_ptr<TypeInfo> typeInfo, ModuleContext* context, bool isPointer) {
    while (typeInfo->type == TypeInfo::TypeName::Custom) {
        if (auto r = context->GetScopeManager().GetNamedTypeAlias(typeInfo->name); r.has_value()) {
            typeInfo = *r;
        } else {
            break;
        }
    }

    llvm::Type* type;
    llvm::DIType* diType;

    switch (typeInfo->type)
    {
    case TypeInfo::TypeName::Aggregate:
        type = nullptr;
        diType = nullptr;
        break;
    case TypeInfo::TypeName::Callable:
        type = nullptr;
        diType = nullptr;
        break;
    case TypeInfo::TypeName::Float:
        type = llvm::Type::getFloatTy(*context->GetTSContext().getContext());
        diType = context->GetDIBasicTypes()->floatDIType;
        break;
    case TypeInfo::TypeName::Bool:
        type = llvm::Type::getInt1Ty(*context->GetTSContext().getContext());
        diType = context->GetDIBasicTypes()->boolDIType;
        break;
    case TypeInfo::TypeName::Int:
        type = llvm::Type::getInt32Ty(*context->GetTSContext().getContext());
        diType = context->GetDIBasicTypes()->intDIType;
        break;
    case TypeInfo::TypeName::Void:
        type = llvm::Type::getVoidTy(*context->GetTSContext().getContext());
        diType = context->GetDIBasicTypes()->voidDIType;
        break;
    case TypeInfo::TypeName::VectorFloat:
        type = llvm::FixedVectorType::get(llvm::Type::getFloatTy(*context->GetTSContext().getContext()), 
            NativeTarget::GetInstance()->GetMaxVectorElementWidth());
        diType = context->GetDIBasicTypes()->vfloatDIType;
        break;
    case TypeInfo::TypeName::VectorBool:
        type = llvm::FixedVectorType::get(llvm::Type::getInt1Ty(*context->GetTSContext().getContext()), 
            NativeTarget::GetInstance()->GetMaxVectorElementWidth());
        diType = context->GetDIBasicTypes()->vboolDIType;
        break;
    case TypeInfo::TypeName::VectorInt:
        type = llvm::FixedVectorType::get(llvm::Type::getInt32Ty(*context->GetTSContext().getContext()), 
            NativeTarget::GetInstance()->GetMaxVectorElementWidth());
        diType = context->GetDIBasicTypes()->vintDIType;
        break;
    case TypeInfo::TypeName::Array:
        {
            if (typeInfo->arguments.size() != 2)
                return std::unexpected{ std::format("Buffer type `{}` expect 2 template arguments but got {} instead.", 
                    ToString(typeInfo), typeInfo->arguments.size()) };
            if (typeInfo->arguments[0]->type != TemplateArgument::TemplateValueType::Typename)
                return std::unexpected{ std::format("Buffer type `{}` first template argument expect a typename but got `{}`.", 
                    ToString(typeInfo), ToString(typeInfo->arguments[0])) };
            if (typeInfo->arguments[1]->type != TemplateArgument::TemplateValueType::Int)
                return std::unexpected{ std::format("Buffer type `{}` second template argument expect a int size but got `{}`.", 
                    ToString(typeInfo), ToString(typeInfo->arguments[1])) };
            if (auto t = Type::Create(typeInfo->arguments[0]->typeInfo, context); t.has_value()) {
                Type elementType = *t;
                int size = typeInfo->arguments[1]->intValue;
                type = llvm::ArrayType::get(elementType.GetLLVMType(), size);
                uint64_t elementSizeInBits = context->GetTSModule().getModuleUnlocked()->getDataLayout().getTypeSizeInBits(elementType.GetLLVMType());
                auto* subrange = context->GetDIBuilder().getOrCreateSubrange(0, size);
                auto subscript = context->GetDIBuilder().getOrCreateArray(subrange);
                diType = context->GetDIBuilder().createArrayType(
                    size * elementSizeInBits,
                    32, elementType.diType, subscript
                );
                diType = context->GetDIBuilder().createTypedef(diType, ToString(typeInfo), context->GetDIBuiltinFile(), 0, nullptr);
            } else {
                return std::unexpected{ t.error() };
            }
        }
        break;
    case TypeInfo::TypeName::Span:
        {
            if (!typeInfo->IsExtern())
                return std::unexpected{ std::format("Buffer type `{}` must be extern.", ToString(typeInfo)) };
            if (typeInfo->arguments.size() != 1)
                return std::unexpected{ std::format("Buffer type `{}` expect 1 template arguments but got {} instead.", 
                    ToString(typeInfo), typeInfo->arguments.size()) };
            if (typeInfo->arguments[0]->type != TemplateArgument::TemplateValueType::Typename)
                return std::unexpected{ std::format("Buffer type `{}` template argument expect a typename but got `{}`.", 
                    ToString(typeInfo), ToString(typeInfo->arguments[0])) };
            if (auto t = Type::Create(typeInfo->arguments[0]->typeInfo, context); t.has_value()) {
                Type elementType = *t;
                uint64_t elementSizeInBits = context->GetTSModule().getModuleUnlocked()->getDataLayout().getTypeSizeInBits(elementType.GetLLVMType());
                llvm::StructType* structType = llvm::StructType::create(
                    { llvm::PointerType::get(elementType.GetLLVMType(), 0), llvm::Type::getInt32Ty(*context->GetTSContext().getContext()) });
                uint64_t structSizeInBits = context->GetTSModule().getModuleUnlocked()->getDataLayout().getTypeSizeInBits(structType);
                uint64_t structAlignInBits = context->GetTSModule().getModuleUnlocked()->getDataLayout().getABITypeAlign(structType).value() * 8;
                type = structType;
                auto elements = context->GetDIBuilder().getOrCreateArray({ elementType.diType, context->GetDIBasicTypes()->intDIType });
                diType = context->GetDIBuilder().createStructType(nullptr, ToString(typeInfo), context->GetDIBuiltinFile(), 0, structSizeInBits,
                    structAlignInBits, llvm::DINode::FlagZero, nullptr, elements);
            } else {
                return std::unexpected{ t.error() };
            }
        }
        break;
    case TypeInfo::TypeName::Custom:
        {
            if (auto t = context->GetScopeManager().GetNamedStructTemplate(typeInfo->name); t.has_value() && typeInfo->arguments.empty())
                return std::unexpected(Error{ std::format("Missing template arguments for generic type `{}`", typeInfo->name) });
            if (!typeInfo->arguments.empty()) {
                if (auto t = context->GetScopeManager().GetNamedStructTemplate(typeInfo->name); t.has_value()) {
                    std::string mangledName = (*t)->Mangle(typeInfo->arguments);
                    if (auto t = context->GetScopeManager().GetNamedType(mangledName); t.has_value()) {
                        type = (*t)->GetType();
                        diType = (*t)->GetDIType();
                        break;
                    }
                    if (auto r = (*t)->Resolve(typeInfo->arguments); r.has_value()) {
                        type = (*r)->GetType();
                        diType = (*r)->GetDIType();
                        break;
                    } else
                        return std::unexpected(r.error());
                } else 
                    return std::unexpected(t.error());
            } else {
                if (auto t = context->GetScopeManager().GetNamedType(typeInfo->name); t.has_value()) {
                    type = (*t)->GetType();
                    diType = (*t)->GetDIType();
                } else
                    return std::unexpected(t.error());
            }
        }
        break;
    default:
        return std::unexpected(Error{ "Invalid typename while parsing type" });
    }

    if (type != nullptr && type->isSized()) {
        typeInfo->rtInfo.sizeInBytes = context->GetTSModule().getModuleUnlocked()->getDataLayout().getTypeSizeInBits(type) / 8;
        typeInfo->rtInfo.alignmentInBytes = context->GetTSModule().getModuleUnlocked()->getDataLayout().getABITypeAlign(type).value();
    }

    return Type{ typeInfo, type, diType, context, isPointer};
}

std::expected<VCL::Type, VCL::Error> VCL::Type::CreatePointerType(Type baseType) {
    llvm::DIType* diType;
    diType = baseType.context->GetDIBuilder().createPointerType(baseType.diType, 64);
    return Type{ baseType.typeInfo, baseType.GetLLVMType()->getPointerTo(), diType, baseType.context, true };
}