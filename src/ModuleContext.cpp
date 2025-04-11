#include "ModuleContext.hpp"



VCL::ModuleContext::ModuleContext(std::string_view name, llvm::LLVMContext* context, std::shared_ptr<Logger> logger) :
    context{ context }, module{ name, *context }, irBuilder{ *context }, sm{}, logger{ logger } {

}

VCL::ModuleContext::~ModuleContext() {

}

llvm::LLVMContext& VCL::ModuleContext::GetContext() {
    return *context;
}

llvm::Module& VCL::ModuleContext::GetModule() {
    return module;
}

llvm::IRBuilder<>& VCL::ModuleContext::GetIRBuilder() {
    return irBuilder;
}

VCL::ScopeManager& VCL::ModuleContext::GetScopeManager() {
    return sm;
}

std::shared_ptr<VCL::Logger> VCL::ModuleContext::GetLogger() {
    return logger;
}