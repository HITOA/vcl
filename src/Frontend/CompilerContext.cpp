#include <VCL/Frontend/CompilerContext.hpp>

#include <VCL/Core/Diagnostic.hpp>
#include <VCL/Core/DiagnosticConsumer.hpp>
#include <VCL/Core/SourceManager.hpp>
#include <VCL/Core/Identifier.hpp>
#include <VCL/Core/Target.hpp>
#include <VCL/Frontend/CompilerInstance.hpp>


VCL::CompilerContext::CompilerContext() : invocation{ std::make_shared<CompilerInvocation>() } {}

VCL::CompilerContext::CompilerContext(std::shared_ptr<CompilerInvocation> invocation)
        : invocation{ invocation } {

}

VCL::CompilerContext::~CompilerContext() {
    
}

VCL::DiagnosticsEngine& VCL::CompilerContext::GetDiagnosticsEngine() {
    return *diagnosticsEngine;
}

VCL::DiagnosticReporter& VCL::CompilerContext::GetDiagnosticReporter() {
    return *diagnosticReporter;
}

bool VCL::CompilerContext::HasDiagnosticEngine() {
    return diagnosticsEngine != nullptr;
}

bool VCL::CompilerContext::CreateDiagnosticEngine() {
    diagnosticsEngine = llvm::makeIntrusiveRefCnt<DiagnosticsEngine>(invocation->GetDiagnosticOptions());
    diagnosticReporter = llvm::makeIntrusiveRefCnt<DiagnosticReporter>(*diagnosticsEngine);
    return true;
}

VCL::SourceManager& VCL::CompilerContext::GetSourceManager() {
    return *sourceManager;
}

bool VCL::CompilerContext::HasSourceManager() {
    return sourceManager != nullptr;
}

bool VCL::CompilerContext::CreateSourceManager() {
    if (!HasDiagnosticEngine())
        return false;
    sourceManager = llvm::makeIntrusiveRefCnt<SourceManager>(GetDiagnosticReporter());
    invocation->GetDiagnosticOptions().GetDiagnosticConsumer()->SetSourceManager(sourceManager.get());
    return true;
}

VCL::IdentifierTable& VCL::CompilerContext::GetIdentifierTable() {
    return *identifierTable;
}

bool VCL::CompilerContext::HasIdentifierTable() {
    return identifierTable != nullptr;
}

bool VCL::CompilerContext::CreateIdentifierTable() {
    identifierTable = llvm::makeIntrusiveRefCnt<IdentifierTable>();
    identifierTable->AddKeywords();
    return true;
}

VCL::Target& VCL::CompilerContext::GetTarget() {
    return *target;
}

bool VCL::CompilerContext::HasTarget() {
    return target != nullptr;
}

bool VCL::CompilerContext::CreateTarget() {
    target = llvm::makeIntrusiveRefCnt<Target>(invocation->GetTargetOptions());
    return true;
}

llvm::orc::ThreadSafeContext& VCL::CompilerContext::GetLLVMContext() {
    return llvmContext;
}

bool VCL::CompilerContext::HasLLVMContext() {
    return llvmContext.withContextDo([](llvm::LLVMContext* ctx){ return ctx != nullptr; });
}

bool VCL::CompilerContext::CreateLLVMContext() {
    llvmContext = llvm::orc::ThreadSafeContext{ std::make_unique<llvm::LLVMContext>() };
    return true;
}

std::shared_ptr<VCL::CompilerInstance> VCL::CompilerContext::CreateInstance() {
    return std::make_shared<CompilerInstance>(*this);
}