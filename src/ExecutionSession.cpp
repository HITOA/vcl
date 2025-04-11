#include <VCL/ExecutionSession.hpp>

#include "ExecutionContext.hpp"
#include "ModuleContext.hpp"


VCL::ExecutionSession::ExecutionSession() {
    context = std::make_unique<ExecutionContext>();
}

VCL::ExecutionSession::~ExecutionSession() {

}

std::unique_ptr<VCL::Module> VCL::ExecutionSession::CreateModule(std::unique_ptr<ASTProgram> program) {
    std::string moduleName = program->source->path.filename().string();
    std::unique_ptr<ModuleContext> moduleContext = std::make_unique<ModuleContext>(moduleName, &context->GetContext(), logger);
    std::unique_ptr<Module> module = std::make_unique<Module>(std::move(moduleContext));
    module->BindProgram(std::move(program));
    return std::move(module);
}

void VCL::ExecutionSession::SetLogger(std::shared_ptr<Logger> logger) {
    this->logger = logger;
}

std::unique_ptr<VCL::ExecutionSession> VCL::ExecutionSession::Create(std::shared_ptr<Logger> logger) {
    std::unique_ptr<ExecutionSession> session = std::make_unique<ExecutionSession>();
    session->SetLogger(logger);
    return std::move(session);
}