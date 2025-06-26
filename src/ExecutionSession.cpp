#include <VCL/ExecutionSession.hpp>

#include "ExecutionContext.hpp"
#include "ModuleContext.hpp"
#include <VCL/NativeTarget.hpp>


VCL::ExecutionSession::ExecutionSession() {
    NativeTarget::GetInstance();
    context = std::make_unique<ExecutionContext>();
}

VCL::ExecutionSession::~ExecutionSession() {

}

std::unique_ptr<VCL::Module> VCL::ExecutionSession::CreateModule(std::unique_ptr<ASTProgram> program) {
    std::string moduleName = program->source->path.filename().string();
    std::unique_ptr<ModuleContext> moduleContext = std::make_unique<ModuleContext>(moduleName, context->GetTSContext(), logger);
    std::unique_ptr<Module> module = std::make_unique<Module>(std::move(moduleContext));
    module->BindProgram(std::move(program));
    return std::move(module);
}

void VCL::ExecutionSession::SubmitModule(std::unique_ptr<Module> module) {
    context->AddModule(std::move(module->context->GetTSModule()));
}

void VCL::ExecutionSession::Clear() {
    context->Clear();
}

void VCL::ExecutionSession::DefineExternSymbolPtr(std::string_view name, void* buffer) {
    context->DefineExternSymbolPtr(name, buffer);
}

void* VCL::ExecutionSession::Lookup(std::string_view name) {
    return context->Lookup(name).getAddress().toPtr<void*>();
}

void VCL::ExecutionSession::SetDumpObject(std::filesystem::path directory, std::string_view identifier) {
    context->SetDumpObject(directory, identifier);
}

void VCL::ExecutionSession::SetDebugInformation(bool enabled) {
    if (enabled)
        context->EnableDebugInformation();
    else
        context->DisableDebugInformation();
}

void VCL::ExecutionSession::SetLogger(std::shared_ptr<Logger> logger) {
    this->logger = logger;
}

std::unique_ptr<VCL::ExecutionSession> VCL::ExecutionSession::Create(std::shared_ptr<Logger> logger) {
    std::unique_ptr<ExecutionSession> session = std::make_unique<ExecutionSession>();
    session->SetLogger(logger);
    return std::move(session);
}