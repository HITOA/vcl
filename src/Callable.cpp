#include "Callable.hpp"

#include "ModuleContext.hpp"

#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_os_ostream.h>


VCL::Function::Function(llvm::Function* value, Type type, Type returnType, std::vector<ArgInfo>& argsType, ModuleContext* context) :
    Callable{ llvm::cast<llvm::Value>(value), type, context }, returnType{ returnType }, argsType{ argsType } {}

std::expected<VCL::Handle<VCL::Value>, VCL::Error> VCL::Function::Call(std::vector<Handle<Value>>& argsv) {
    llvm::Function* callee = GetLLVMFunction();
    std::vector<llvm::Value*> llvmargsv(argsv.size());
    for (size_t i = 0; i < argsv.size(); ++i)
        llvmargsv[i] = argsv[i]->GetLLVMValue();
    
    llvm::Value* llvmReturnValue = GetModuleContext()->GetIRBuilder().CreateCall(callee, llvmargsv);
    return Value::Create(llvmReturnValue, returnType, GetModuleContext());
}

VCL::Type VCL::Function::GetReturnType() {
    return returnType;
}

uint32_t VCL::Function::GetArgCount() {
    return argsType.size();
}

VCL::Type VCL::Function::GetArgType(uint32_t index) {
    return argsType[index].type;
}

bool VCL::Function::CheckArgCount(uint32_t count) {
    return count == argsType.size();
}

bool VCL::Function::CheckArgType(uint32_t index, Type type) {
    return argsType[index].type == type;
}

VCL::CallableType VCL::Function::GetCallableType() {
    return CallableType::Function;
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
    std::stringstream sstream{};
    llvm::raw_os_ostream llvmStream{ sstream };
    if (llvm::verifyFunction(*GetLLVMFunction(), &llvmStream)) {
        llvmStream.flush();
        throw std::runtime_error{ sstream.str() };
    }
}

std::expected<VCL::Handle<VCL::Function>, VCL::Error> VCL::Function::Create(
    Type returnType, std::vector<ArgInfo>& argsInfo, std::string_view name, ModuleContext* context) {
    
    std::vector<llvm::Type*> argsType( argsInfo.size() );
    for (size_t i = 0; i < argsInfo.size(); ++i)
        argsType[i] = argsInfo[i].type.GetLLVMType();

    llvm::FunctionType* functionType = llvm::FunctionType::get(returnType.GetLLVMType(), argsType, false);
    llvm::Function* function = llvm::Function::Create(functionType, llvm::GlobalValue::ExternalLinkage, name, context->GetTSModule().getModuleUnlocked());

    for (size_t i = 0; i < argsInfo.size(); ++i)
        function->getArg(i)->setName(argsInfo[i].name);

    if (auto fType = Type::CreateFromLLVMType(function->getFunctionType(), context); fType.has_value())
        return MakeHandle<Function>(function, *fType, returnType, argsInfo, context);
    else
        return std::unexpected(fType.error());
}