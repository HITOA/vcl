#include "Type.hpp"

#include <VCL/Debug.hpp>
#include <VCL/NativeTarget.hpp>

#include "ModuleContext.hpp"
#include "Utility.hpp"

#include <iostream>


VCL::Type::Type() :
    typeInfo{}, type{ nullptr }, context{ nullptr } {}

VCL::Type::Type(std::shared_ptr<TypeInfo> typeInfo, llvm::Type* type, ModuleContext* context) :
    typeInfo{ typeInfo }, type{ type }, context{ context } {}

std::shared_ptr<VCL::TypeInfo> VCL::Type::GetTypeInfo() const {
    return typeInfo;
}

llvm::Type* VCL::Type::GetLLVMType() const {
    return type;
}

bool VCL::Type::operator==(Type& rhs) const {
    return this->typeInfo->type == rhs.typeInfo->type && this->typeInfo->name == rhs.typeInfo->name;
}

bool VCL::Type::operator!=(Type& rhs) const {
    return !(*this != rhs);
}

std::expected<VCL::Type, VCL::Error> VCL::Type::Create(std::shared_ptr<TypeInfo> typeInfo, ModuleContext* context) {
    while (typeInfo->type == TypeInfo::TypeName::Custom) {
        if (auto r = context->GetScopeManager().GetNamedTypeAlias(typeInfo->name); r.has_value()) {
            typeInfo = *r;
        } else {
            break;
        }
    }

    llvm::Type* type;
    switch (typeInfo->type)
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
                type = llvm::ArrayType::get(elementType.GetLLVMType(), typeInfo->arguments[1]->intValue);
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
                llvm::StructType* structType = llvm::StructType::create(
                    { llvm::PointerType::get(elementType.GetLLVMType(), 0), llvm::Type::getInt32Ty(*context->GetTSContext().getContext()) });
                type = structType;
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
                        break;
                    }
                    if (auto r = (*t)->Resolve(typeInfo->arguments); r.has_value()) {
                        type = (*r)->GetType();
                        break;
                    } else
                        return std::unexpected(r.error());
                } else 
                    return std::unexpected(t.error());
            } else {
                if (auto t = context->GetScopeManager().GetNamedType(typeInfo->name); t.has_value())
                    type = (*t)->GetType();
                else
                    return std::unexpected(t.error());
            }
        }
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

    std::shared_ptr<TypeInfo> typeInfo = std::make_shared<TypeInfo>();

    if (trueType->isStructTy()) {
        typeInfo->type = TypeInfo::TypeName::Custom;
        typeInfo->name = trueType->getStructName();
        return Type{ typeInfo, type, context };
    }
    
    if (trueType->isFunctionTy()) {
        typeInfo->type = TypeInfo::TypeName::Callable;
        return Type{ typeInfo, type, context };
    }

    if (trueType->isFloatTy())
        typeInfo->type = TypeInfo::TypeName::Float;
    else if (trueType->isIntegerTy()) {
        uint32_t width = trueType->getIntegerBitWidth();
        if (width == 1)
            typeInfo->type = TypeInfo::TypeName::Bool;
        else if (width == 32)
            typeInfo->type = TypeInfo::TypeName::Int;
        else
            return std::unexpected(Error{ "Unsupported integer bit width" });
    } else if (trueType->isVoidTy())
        typeInfo->type = TypeInfo::TypeName::Void;
    else
        return std::unexpected(Error{ "Unsupported LLVM type" });

    if (type->isVectorTy())
        typeInfo->type = (TypeInfo::TypeName)((uint32_t)typeInfo->type + 
            (uint32_t)TypeInfo::TypeName::VectorFloat - (uint32_t)TypeInfo::TypeName::Float);
    
    return Type{ typeInfo, type, context };
}