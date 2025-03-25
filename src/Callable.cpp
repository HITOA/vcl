#include "Callable.hpp"

#include <llvm/IR/Verifier.h>


VCL::Function::Function(llvm::Function* value, Type type, Type returnType, std::vector<ArgInfo>& argsType, ModuleContext* context) :
    Callable{ llvm::cast<llvm::Value>(value), type, context }, returnType{ returnType }, argsType{ argsType } {}

VCL::Type VCL::Function::GetReturnType() const {
    return returnType;
}

const std::vector<VCL::Function::ArgInfo>& VCL::Function::GetArgsInfo() const {
    return argsType;
}

llvm::Function* VCL::Function::GetLLVMFunction() const {
    return llvm::cast<llvm::Function>(GetLLVMValue());
}

bool VCL::Function::HasStorage() const {
    return !GetLLVMFunction()->empty();
}

void VCL::Function::Verify() const {
    llvm::verifyFunction(*GetLLVMFunction());
}

std::expected<VCL::Handle<VCL::Function>, VCL::Error> VCL::Function::Create(
    Type returnType, std::vector<ArgInfo>& argsInfo, std::string_view name, ModuleContext* context) {
    
    std::vector<llvm::Type*> argsType( argsInfo.size() );
    for (size_t i = 0; i < argsInfo.size(); ++i)
        argsType[i] = argsInfo[i].type.GetLLVMType();

    llvm::FunctionType* functionType = llvm::FunctionType::get(returnType.GetLLVMType(), argsType, false);
    llvm::Function* function = llvm::Function::Create(functionType, llvm::GlobalValue::ExternalLinkage, name, context->GetModule());

    for (size_t i = 0; i < argsInfo.size(); ++i)
        function->getArg(i)->setName(argsInfo[i].name);

    return MakeHandle<Function>(function, Type::CreateFromLLVMType(function->getType(), context), returnType, argsInfo, context);
}