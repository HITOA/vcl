#include "Value.hpp"

#include "ModuleContext.hpp"
#include "Utility.hpp"
#include "NativeTarget.hpp"

#include <VCL/Debug.hpp>

#include <iostream>


VCL::Value::Value() : value{ nullptr }, type{ TypeInfo{}, nullptr, nullptr }, context{ nullptr } {}

VCL::Value::Value(llvm::Value* value, Type type, ModuleContext* context) :
    value{ value }, type{ type }, context{ context } {}

std::expected<VCL::Handle<VCL::Value>, VCL::Error> VCL::Value::Load() {
    llvm::Value* loadedValue = value;
    std::string loadedValueName = value->getName().str() + "_loaded";

    if (loadedValue->getType()->isPointerTy())
        loadedValue = context->GetIRBuilder().CreateLoad(type.GetLLVMType(), loadedValue, loadedValueName);

    return Value::Create(loadedValue, type, context);
}

void VCL::Value::Store(Handle<Value> value) {
    context->GetIRBuilder().CreateStore(value->GetLLVMValue(), this->value);
}

bool VCL::Value::IsAssignableFrom(Handle<Value> value) const {
    return value->GetType().GetTypeInfo().type == type.GetTypeInfo().type && !type.GetTypeInfo().IsConst();
}

std::expected<VCL::Handle<VCL::Value>, VCL::Error> VCL::Value::Splat() {
    if (value->getType()->isPointerTy())
        return std::unexpected(Error{ "Cannot splat pointer type" });
    
    switch (type.GetTypeInfo().type) {
        case TypeInfo::TypeName::Float:
        case TypeInfo::TypeName::Bool:
        case TypeInfo::TypeName::Int:
            break;
        default:
            return Value::Create(value, type, context);
    }
    
    llvm::Value* v = context->GetIRBuilder().CreateVectorSplat(NativeTarget::GetInstance()->GetMaxVectorElementWidth(), value);
    if (auto t = Type::CreateFromLLVMType(v->getType(), context); t.has_value()) {
        return Value::Create(v, *t, context);
    } else {
        return std::unexpected(t.error());
    }
}

std::expected<VCL::Handle<VCL::Value>, VCL::Error> VCL::Value::Cast(Type type) {
    if (this->type == type)
        return Value::Create(value, type, context);
    
    if (value->getType()->isPointerTy())
        return std::unexpected(Error{ "Cannot cast pointer type to non pointer type" });

    if (this->type.GetTypeInfo().type == TypeInfo::TypeName::Float) {
        if (type.GetTypeInfo().type == TypeInfo::TypeName::Bool)
            return Value::Create(context->GetIRBuilder().CreateFPToSI(value, type.GetLLVMType()), type, context);
        if (type.GetTypeInfo().type == TypeInfo::TypeName::Int)
            return Value::Create(context->GetIRBuilder().CreateFPToSI(value, type.GetLLVMType()), type, context);
        if (type.GetTypeInfo().type == TypeInfo::TypeName::VectorFloat) {
            return Value::Create(context->GetIRBuilder().CreateVectorSplat(NativeTarget::GetInstance()->GetMaxVectorElementWidth(), value), type, context);
        }
    }

    if (this->type.GetTypeInfo().type == TypeInfo::TypeName::Bool) {
        if (type.GetTypeInfo().type == TypeInfo::TypeName::Float)
            return Value::Create(context->GetIRBuilder().CreateSIToFP(value, type.GetLLVMType()), type, context);
        if (type.GetTypeInfo().type == TypeInfo::TypeName::Int)
            return Value::Create(context->GetIRBuilder().CreateZExt(value, type.GetLLVMType()), type, context);
        if (type.GetTypeInfo().type == TypeInfo::TypeName::VectorFloat)
            return Value::Create(context->GetIRBuilder().CreateVectorSplat(NativeTarget::GetInstance()->GetMaxVectorElementWidth(), 
                context->GetIRBuilder().CreateSIToFP(value, type.GetLLVMType())), type, context);
    }

    if (this->type.GetTypeInfo().type == TypeInfo::TypeName::Int) {
        if (type.GetTypeInfo().type == TypeInfo::TypeName::Float)
            return Value::Create(context->GetIRBuilder().CreateSIToFP(value, type.GetLLVMType()), type, context);
        if (type.GetTypeInfo().type == TypeInfo::TypeName::Bool)
            return Value::Create(context->GetIRBuilder().CreateTrunc(value, type.GetLLVMType()), type, context);
        if (type.GetTypeInfo().type == TypeInfo::TypeName::VectorFloat)
            return Value::Create(context->GetIRBuilder().CreateVectorSplat(NativeTarget::GetInstance()->GetMaxVectorElementWidth(), 
                context->GetIRBuilder().CreateSIToFP(value, type.GetLLVMType())), type, context);
    }

    return std::unexpected(Error{ std::format("Cannot cast value of type `{}` to type `{}`: cast not supported.", 
        ToString(this->type.GetTypeInfo()), ToString(type.GetTypeInfo())) });
}

bool VCL::Value::IsValid() const {
    return value != nullptr;
}

llvm::Value* VCL::Value::GetLLVMValue() const {
    return value;
}

VCL::Type VCL::Value::GetType() const {
    return type;
}

VCL::ModuleContext* VCL::Value::GetModuleContext() const {
    return context;
}

std::expected<VCL::Handle<VCL::Value>, VCL::Error> VCL::Value::Create(llvm::Value* value, Type type, ModuleContext* context) {
    return MakeHandle<Value>(value, type, context);
}

std::expected<VCL::Handle<VCL::Value>, VCL::Error> VCL::Value::CreateGlobalVariable(Type type, Handle<Value> initializer, ModuleContext* context, const char* name) {
    llvm::Constant* initializerValue = nullptr;
    bool isExtern = type.GetTypeInfo().IsExtern();;

    if (initializer) {
        if (llvm::isa<llvm::Constant>(initializer->GetLLVMValue()))
            initializerValue = llvm::cast<llvm::Constant>(initializer->GetLLVMValue());
        else
            std::unexpected(Error{ "Global variable initializer must be const" });
    } else if (!isExtern) {
        initializerValue = llvm::ConstantStruct::getNullValue(type.GetLLVMType());
    }
    
    llvm::GlobalVariable* value = new llvm::GlobalVariable{
        *context->GetTSModule().getModuleUnlocked(),
        type.GetLLVMType(),
        type.GetTypeInfo().IsConst(),
        isExtern ? llvm::GlobalValue::ExternalLinkage : llvm::GlobalValue::PrivateLinkage,
        initializerValue, name
    };

    value->setAlignment(llvm::Align(NativeTarget::GetInstance()->GetMaxVectorByteWidth()));

    return Value::Create(value, type, context);
}

std::expected<VCL::Handle<VCL::Value>, VCL::Error> VCL::Value::CreateLocalVariable(Type type, Handle<Value> initializer, ModuleContext* context, const char* name) {
    llvm::BasicBlock* bb = context->GetIRBuilder().GetInsertBlock();
    llvm::AllocaInst* alloca;
    {
        llvm::IRBuilder<>::InsertPointGuard ipGuard{ context->GetIRBuilder() };
        context->GetIRBuilder().SetInsertPoint(bb->getFirstInsertionPt());
        context->GetIRBuilder().SetCurrentDebugLocation(llvm::DebugLoc());
        alloca = context->GetIRBuilder().CreateAlloca(type.GetLLVMType(), nullptr, name);
    }
    if (initializer)
        context->GetIRBuilder().CreateStore(initializer->GetLLVMValue(), alloca);
    return Value::Create(alloca, type, context);
}

std::expected<VCL::Handle<VCL::Value>, VCL::Error> VCL::Value::CreateConstantInt32(int value, ModuleContext* context) {
    TypeInfo typeInfo{};
    typeInfo.type = TypeInfo::TypeName::Int;
    typeInfo.qualifiers = TypeInfo::QualifierFlag::Const;
    
    auto type = Type::Create(typeInfo, context);

    if (!type.has_value())
        std::unexpected(type.error());

    llvm::Constant* constant = llvm::ConstantInt::get(type->GetLLVMType(), value, true);

    return Value::Create(constant, *type, context);
}

std::expected<VCL::Handle<VCL::Value>, VCL::Error> VCL::Value::CreateConstantInt1(bool value, ModuleContext* context) {
    TypeInfo typeInfo{};
    typeInfo.type = TypeInfo::TypeName::Bool;
    typeInfo.qualifiers = TypeInfo::QualifierFlag::Const;
    
    auto type = Type::Create(typeInfo, context);

    if (!type.has_value())
        std::unexpected(type.error());

    llvm::Constant* constant = llvm::ConstantInt::get(type->GetLLVMType(), llvm::APInt{ 1, value, true });

    return Value::Create(constant, *type, context);
}

std::expected<VCL::Handle<VCL::Value>, VCL::Error> VCL::Value::CreateConstantFloat(float value, ModuleContext* context) {
    TypeInfo typeInfo{};
    typeInfo.type = TypeInfo::TypeName::Float;
    typeInfo.qualifiers = TypeInfo::QualifierFlag::Const;
    
    auto type = Type::Create(typeInfo, context);

    if (!type.has_value())
        std::unexpected(type.error());

    llvm::Constant* constant = llvm::ConstantFP::get(type->GetLLVMType(), llvm::APFloat{ value });

    return Value::Create(constant, *type, context);
}

std::expected<VCL::Handle<VCL::Value>, VCL::Error> VCL::Value::CreateInvalid() {
    return MakeHandle<Value>();
}