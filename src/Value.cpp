#include "Value.hpp"

#include "ModuleContext.hpp"
#include "Utility.hpp"

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

std::expected<VCL::Handle<VCL::Value>, VCL::Error> VCL::Value::Cast(Type type) {
    if (this->type == type)
        return Value::Create(value, type, context);
    
    if (value->getType()->isPointerTy())
        std::unexpected(Error{ "Cannot cast pointer type to non pointer type" });

    if (this->type.GetTypeInfo().type == TypeInfo::TypeName::FLOAT) {
        if (type.GetTypeInfo().type == TypeInfo::TypeName::BOOLEAN)
            return Value::Create(context->GetIRBuilder().CreateFPToSI(value, type.GetLLVMType()), type, context);
        if (type.GetTypeInfo().type == TypeInfo::TypeName::INT)
            return Value::Create(context->GetIRBuilder().CreateFPToSI(value, type.GetLLVMType()), type, context);
        if (type.GetTypeInfo().type == TypeInfo::TypeName::VFLOAT)
            return Value::Create(context->GetIRBuilder().CreateVectorSplat(GetMaxVectorElementWidth(sizeof(float)), value), type, context);
    }

    if (this->type.GetTypeInfo().type == TypeInfo::TypeName::BOOLEAN) {
        if (type.GetTypeInfo().type == TypeInfo::TypeName::FLOAT)
            return Value::Create(context->GetIRBuilder().CreateSIToFP(value, type.GetLLVMType()), type, context);
        if (type.GetTypeInfo().type == TypeInfo::TypeName::INT)
            return Value::Create(context->GetIRBuilder().CreateZExt(value, type.GetLLVMType()), type, context);
        if (type.GetTypeInfo().type == TypeInfo::TypeName::VFLOAT)
            return Value::Create(context->GetIRBuilder().CreateVectorSplat(GetMaxVectorElementWidth(sizeof(float)), 
                context->GetIRBuilder().CreateSIToFP(value, type.GetLLVMType())), type, context);
    }

    if (this->type.GetTypeInfo().type == TypeInfo::TypeName::INT) {
        if (type.GetTypeInfo().type == TypeInfo::TypeName::FLOAT)
            return Value::Create(context->GetIRBuilder().CreateSIToFP(value, type.GetLLVMType()), type, context);
        if (type.GetTypeInfo().type == TypeInfo::TypeName::BOOLEAN)
            return Value::Create(context->GetIRBuilder().CreateTrunc(value, type.GetLLVMType()), type, context);
        if (type.GetTypeInfo().type == TypeInfo::TypeName::VFLOAT)
            return Value::Create(context->GetIRBuilder().CreateVectorSplat(GetMaxVectorElementWidth(sizeof(float)), 
                context->GetIRBuilder().CreateSIToFP(value, type.GetLLVMType())), type, context);
    }

    return std::unexpected(Error{ "Invalid cast" });
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
    bool isExtern = false;

    if (initializer) {
        if (llvm::isa<llvm::Constant>(initializer->GetLLVMValue()))
            initializerValue = llvm::cast<llvm::Constant>(initializer->GetLLVMValue());
        else
            std::unexpected(Error{ "Global variable initializer must be const" });
    }

    isExtern = type.GetTypeInfo().IsExtern() || !initializer;
    
    llvm::GlobalVariable* value = new llvm::GlobalVariable{
        context->GetModule(),
        type.GetLLVMType(),
        type.GetTypeInfo().IsConst() || type.GetTypeInfo().IsInput(),
        isExtern ? llvm::GlobalValue::ExternalLinkage : llvm::GlobalValue::PrivateLinkage,
        initializerValue, name
    };

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
    typeInfo.type = TypeInfo::TypeName::INT;
    typeInfo.qualifiers = TypeInfo::QualifierFlag::CONST;
    
    auto type = Type::Create(typeInfo, context);

    if (!type.has_value())
        std::unexpected(type.error());

    llvm::Constant* constant = llvm::ConstantInt::get(type->GetLLVMType(), value, true);

    return Value::Create(constant, *type, context);
}

std::expected<VCL::Handle<VCL::Value>, VCL::Error> VCL::Value::CreateConstantInt1(bool value, ModuleContext* context) {
    TypeInfo typeInfo{};
    typeInfo.type = TypeInfo::TypeName::BOOLEAN;
    typeInfo.qualifiers = TypeInfo::QualifierFlag::CONST;
    
    auto type = Type::Create(typeInfo, context);

    if (!type.has_value())
        std::unexpected(type.error());

    llvm::Constant* constant = llvm::ConstantInt::get(type->GetLLVMType(), llvm::APInt{ 1, value, true });

    return Value::Create(constant, *type, context);
}

std::expected<VCL::Handle<VCL::Value>, VCL::Error> VCL::Value::CreateConstantFloat(float value, ModuleContext* context) {
    TypeInfo typeInfo{};
    typeInfo.type = TypeInfo::TypeName::FLOAT;
    typeInfo.qualifiers = TypeInfo::QualifierFlag::CONST;
    
    auto type = Type::Create(typeInfo, context);

    if (!type.has_value())
        std::unexpected(type.error());

    llvm::Constant* constant = llvm::ConstantFP::get(type->GetLLVMType(), llvm::APFloat{ value });

    return Value::Create(constant, *type, context);
}

std::expected<VCL::Handle<VCL::Value>, VCL::Error> VCL::Value::CreateInvalid() {
    return MakeHandle<Value>();
}