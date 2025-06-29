#include "Value.hpp"

#include "ModuleContext.hpp"
#include "Utility.hpp"

#include <VCL/NativeTarget.hpp>
#include <VCL/Debug.hpp>

#include <iostream>


VCL::Value::Value() : value{ nullptr }, type{ nullptr, nullptr, nullptr, nullptr, false }, context{ nullptr } {}

VCL::Value::Value(llvm::Value* value, Type type, ModuleContext* context) :
    value{ value }, type{ type }, context{ context } {}

std::expected<VCL::Handle<VCL::Value>, VCL::Error> VCL::Value::Load() {
    llvm::Value* loadedValue = value;
    std::string loadedValueName = value->getName().str() + "_loaded";

    if (loadedValue->getType()->isPointerTy())
        loadedValue = context->GetIRBuilder().CreateLoad(type.GetLLVMType(), loadedValue, loadedValueName);
    if (auto newType = Type::Create(type.GetTypeInfo(), context); newType.has_value())
        return Value::Create(loadedValue, *newType, context);
    else
        return std::unexpected{ newType.error() };
}

std::optional<VCL::Error> VCL::Value::Store(Handle<Value> value) {
    if (type.GetTypeInfo()->IsConst())
        return Error{ "You cannot assign to a variable that is const." };
    context->GetIRBuilder().CreateStore(value->GetLLVMValue(), this->value);
    return {};
}

std::expected<VCL::Handle<VCL::Value>, VCL::Error> VCL::Value::Splat() {
    if (value->getType()->isPointerTy())
        return std::unexpected(Error{ "Cannot splat pointer type" });
    
    std::shared_ptr<TypeInfo> typeInfo = std::make_shared<TypeInfo>();
    typeInfo->qualifiers = type.GetTypeInfo()->qualifiers;

    switch (type.GetTypeInfo()->type) {
        case TypeInfo::TypeName::Float:
            typeInfo->type = TypeInfo::TypeName::VectorFloat;
            break;
        case TypeInfo::TypeName::Bool:
            typeInfo->type = TypeInfo::TypeName::VectorBool;
            break;
        case TypeInfo::TypeName::Int:
            typeInfo->type = TypeInfo::TypeName::VectorInt;
            break;
        default:
            return Value::Create(value, type, context);
    }
    
    llvm::Value* v = context->GetIRBuilder().CreateVectorSplat(NativeTarget::GetInstance()->GetMaxVectorElementWidth(), value);
    if (auto t = Type::Create(typeInfo, context); t.has_value()) {
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

    if (this->type.GetTypeInfo()->type == TypeInfo::TypeName::Float) {
        if (type.GetTypeInfo()->type == TypeInfo::TypeName::Bool)
            return Value::Create(context->GetIRBuilder().CreateFPToSI(value, type.GetLLVMType()), type, context);
        if (type.GetTypeInfo()->type == TypeInfo::TypeName::Int)
            return Value::Create(context->GetIRBuilder().CreateFPToSI(value, type.GetLLVMType()), type, context);
        if (type.GetTypeInfo()->type == TypeInfo::TypeName::VectorFloat) {
            return Value::Create(context->GetIRBuilder().CreateVectorSplat(NativeTarget::GetInstance()->GetMaxVectorElementWidth(), value), type, context);
        }
    }

    if (this->type.GetTypeInfo()->type == TypeInfo::TypeName::Bool) {
        if (type.GetTypeInfo()->type == TypeInfo::TypeName::Float)
            return Value::Create(context->GetIRBuilder().CreateSIToFP(value, type.GetLLVMType()), type, context);
        if (type.GetTypeInfo()->type == TypeInfo::TypeName::Int)
            return Value::Create(context->GetIRBuilder().CreateZExt(value, type.GetLLVMType()), type, context);
        if (type.GetTypeInfo()->type == TypeInfo::TypeName::VectorFloat)
            return Value::Create(context->GetIRBuilder().CreateVectorSplat(NativeTarget::GetInstance()->GetMaxVectorElementWidth(), 
                context->GetIRBuilder().CreateSIToFP(value, type.GetLLVMType())), type, context);
    }

    if (this->type.GetTypeInfo()->type == TypeInfo::TypeName::Int) {
        if (type.GetTypeInfo()->type == TypeInfo::TypeName::Float)
            return Value::Create(context->GetIRBuilder().CreateSIToFP(value, type.GetLLVMType()), type, context);
        if (type.GetTypeInfo()->type == TypeInfo::TypeName::Bool)
            return Value::Create(context->GetIRBuilder().CreateTrunc(value, type.GetLLVMType()), type, context);
        if (type.GetTypeInfo()->type == TypeInfo::TypeName::VectorInt)
            return Value::Create(context->GetIRBuilder().CreateVectorSplat(NativeTarget::GetInstance()->GetMaxVectorElementWidth(), value), type, context);
        if (type.GetTypeInfo()->type == TypeInfo::TypeName::VectorFloat)
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

std::expected<VCL::Handle<VCL::Value>, VCL::Error> VCL::Value::CreateGlobalVariable(Type type, Handle<Value> initializer, ModuleContext* context, const char* name,
        llvm::DIFile* file, uint32_t line, uint32_t position) {
    llvm::Constant* initializerValue = nullptr;
    bool isExtern = type.GetTypeInfo()->IsExtern();;

    if (initializer) {
        if (llvm::isa<llvm::Constant>(initializer->GetLLVMValue()))
            initializerValue = llvm::cast<llvm::Constant>(initializer->GetLLVMValue());
        else
            return std::unexpected(Error{ "Global variable initializer must be const" });
    } else if (!isExtern) {
        initializerValue = llvm::ConstantStruct::getNullValue(type.GetLLVMType());
    }
    
    llvm::GlobalVariable* value = new llvm::GlobalVariable{
        *context->GetTSModule().getModuleUnlocked(),
        type.GetLLVMType(),
        type.GetTypeInfo()->IsConst(),
        llvm::GlobalValue::ExternalLinkage,
        initializerValue, name
    };

    if (file) {
        llvm::DIGlobalVariableExpression* diInfo = context->GetDIBuilder().createGlobalVariableExpression(
            file, name, name, file, line + 1, type.GetDIType(), !isExtern, initializerValue != nullptr);
        value->addDebugInfo(diInfo);
    }

    value->setAlignment(llvm::Align(NativeTarget::GetInstance()->GetMaxVectorByteWidth()));
    if (!isExtern)
        value->setDSOLocal(true);
    type.SetPointer();

    return Value::Create(value, type, context);
}

std::expected<VCL::Handle<VCL::Value>, VCL::Error> VCL::Value::CreateLocalVariable(Type type, Handle<Value> initializer, ModuleContext* context, const char* name,
    llvm::DIFile* file, uint32_t line, uint32_t position) {
    llvm::BasicBlock* bb = context->GetIRBuilder().GetInsertBlock();
    llvm::AllocaInst* alloca;

    {
        llvm::IRBuilder<>::InsertPointGuard ipGuard{ context->GetIRBuilder() };
        context->GetIRBuilder().SetInsertPoint(bb->getFirstInsertionPt());
        context->GetIRBuilder().SetCurrentDebugLocation(llvm::DebugLoc());
        alloca = context->GetIRBuilder().CreateAlloca(type.GetLLVMType(), nullptr, name);
    }

    if (llvm::DIScope* scope = context->GetScopeManager().GetCurrentDebugInformationScope()) {
        llvm::DILocalVariable* diVariable = context->GetDIBuilder().createAutoVariable(scope, name, file, line + 1, type.GetDIType(), true);
        context->GetDIBuilder().insertDeclare(alloca, diVariable, 
            context->GetDIBuilder().createExpression(), context->GetIRBuilder().getCurrentDebugLocation(), bb);
    }

    if (initializer) {
        if (auto t = initializer->Cast(type); t.has_value())
            context->GetIRBuilder().CreateStore((*t)->GetLLVMValue(), alloca);
        else
            return std::unexpected(t.error());
    }

    type.SetPointer();
    return Value::Create(alloca, type, context);
}

std::expected<VCL::Handle<VCL::Value>, VCL::Error> VCL::Value::CreateConstantInt32(int value, ModuleContext* context) {
    std::shared_ptr<TypeInfo> typeInfo = std::make_shared<TypeInfo>();
    typeInfo->type = TypeInfo::TypeName::Int;
    typeInfo->qualifiers = TypeInfo::QualifierFlag::Const;
    
    auto type = Type::Create(typeInfo, context);

    if (!type.has_value())
        std::unexpected(type.error());

    llvm::Constant* constant = llvm::ConstantInt::get(type->GetLLVMType(), value, true);

    return Value::Create(constant, *type, context);
}

std::expected<VCL::Handle<VCL::Value>, VCL::Error> VCL::Value::CreateConstantInt1(bool value, ModuleContext* context) {
    std::shared_ptr<TypeInfo> typeInfo = std::make_shared<TypeInfo>();
    typeInfo->type = TypeInfo::TypeName::Bool;
    typeInfo->qualifiers = TypeInfo::QualifierFlag::Const;
    
    auto type = Type::Create(typeInfo, context);

    if (!type.has_value())
        std::unexpected(type.error());

    llvm::Constant* constant = llvm::ConstantInt::get(type->GetLLVMType(), llvm::APInt{ 1, value, true });

    return Value::Create(constant, *type, context);
}

std::expected<VCL::Handle<VCL::Value>, VCL::Error> VCL::Value::CreateConstantFloat(float value, ModuleContext* context) {
    std::shared_ptr<TypeInfo> typeInfo = std::make_shared<TypeInfo>();
    typeInfo->type = TypeInfo::TypeName::Float;
    typeInfo->qualifiers = TypeInfo::QualifierFlag::Const;
    
    auto type = Type::Create(typeInfo, context);

    if (!type.has_value())
        std::unexpected(type.error());

    llvm::Constant* constant = llvm::ConstantFP::get(type->GetLLVMType(), llvm::APFloat{ value });

    return Value::Create(constant, *type, context);
}

std::expected<VCL::Handle<VCL::Value>, VCL::Error> VCL::Value::CreateInvalid() {
    return MakeHandle<Value>();
}