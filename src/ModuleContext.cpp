#include "ModuleContext.hpp"



VCL::ModuleContext::ModuleContext(std::string_view name, llvm::orc::ThreadSafeContext context, std::shared_ptr<Logger> logger) :
    module{ std::make_unique<llvm::Module>(name, *context.getContext()), context }, irBuilder{ *context.getContext() }, sm{}, logger{ logger } {
    
}

VCL::ModuleContext::~ModuleContext() {

}

llvm::orc::ThreadSafeContext VCL::ModuleContext::GetTSContext() {
    return module.getContext();
}

llvm::orc::ThreadSafeModule& VCL::ModuleContext::GetTSModule() {
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