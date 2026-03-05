#include <VCL/Frontend/CompilerContext.hpp>

#include <VCL/Core/Diagnostic.hpp>
#include <VCL/Core/DiagnosticConsumer.hpp>
#include <VCL/Core/SourceManager.hpp>
#include <VCL/Core/Identifier.hpp>
#include <VCL/Core/Attribute.hpp>
#include <VCL/Core/Directive.hpp>
#include <VCL/Core/Target.hpp>
#include <VCL/AST/TypeCache.hpp>
#include <VCL/Frontend/CompilerInstance.hpp>
#include <VCL/Frontend/ModuleCache.hpp>

#include <assert.h>


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

void VCL::CompilerContext::CreateDiagnosticEngine() {
    diagnosticsEngine = llvm::makeIntrusiveRefCnt<DiagnosticsEngine>(invocation->GetDiagnosticOptions());
    diagnosticReporter = llvm::makeIntrusiveRefCnt<DiagnosticReporter>(*diagnosticsEngine);
}

void VCL::CompilerContext::CopyDiagnosticEngine(CompilerContext& other) {
    diagnosticsEngine = other.diagnosticsEngine;
    diagnosticReporter = other.diagnosticReporter;
}

VCL::SourceManager& VCL::CompilerContext::GetSourceManager() {
    return *sourceManager;
}

bool VCL::CompilerContext::HasSourceManager() {
    return sourceManager != nullptr;
}

void VCL::CompilerContext::CreateSourceManager() {
    assert(HasDiagnosticEngine() && "missing diagnostic engine");
    sourceManager = llvm::makeIntrusiveRefCnt<SourceManager>(GetDiagnosticReporter());
    invocation->GetDiagnosticOptions().GetDiagnosticConsumer()->SetSourceManager(sourceManager.get());
}

void VCL::CompilerContext::CopySourceManager(CompilerContext& other) {
    sourceManager = other.sourceManager;
}

VCL::IdentifierTable& VCL::CompilerContext::GetIdentifierTable() {
    return *identifierTable;
}

bool VCL::CompilerContext::HasIdentifierTable() {
    return identifierTable != nullptr;
}

void VCL::CompilerContext::CreateIdentifierTable() {
    identifierTable = llvm::makeIntrusiveRefCnt<IdentifierTable>();
    identifierTable->AddKeywords();
}

void VCL::CompilerContext::CopyIdentifierTable(CompilerContext& other) {
    identifierTable = other.identifierTable;
}

VCL::AttributeTable& VCL::CompilerContext::GetAttributeTable() {
    return *attributeTable;
}

bool VCL::CompilerContext::HasAttributeTable() {
    return attributeTable != nullptr;
}

void VCL::CompilerContext::CreateAttributeTable() {
    assert(HasIdentifierTable() && "missing identifier table");
    attributeTable = llvm::makeIntrusiveRefCnt<AttributeTable>();
    attributeTable->AddDefaults(GetIdentifierTable());
}

void VCL::CompilerContext::CopyAttributeTable(CompilerContext& other) {
    attributeTable = other.attributeTable;
}

VCL::DirectiveRegistry& VCL::CompilerContext::GetDirectiveRegistry() {
    return *directiveRegistry;
}

bool VCL::CompilerContext::HasDirectiveRegistry() {
    return directiveRegistry != nullptr;
}

void VCL::CompilerContext::CreateDirectiveRegistry() {
    directiveRegistry = llvm::makeIntrusiveRefCnt<DirectiveRegistry>();
}

void VCL::CompilerContext::CopyDirectiveRegistry(CompilerContext& other) {
    directiveRegistry = other.directiveRegistry;
}

VCL::Target& VCL::CompilerContext::GetTarget() {
    return *target;
}

bool VCL::CompilerContext::HasTarget() {
    return target != nullptr;
}

void VCL::CompilerContext::CreateTarget() {
    target = llvm::makeIntrusiveRefCnt<Target>(invocation->GetTargetOptions());
}

void VCL::CompilerContext::CopyTarget(CompilerContext& other) {
    target = other.target;
}

VCL::TypeCache& VCL::CompilerContext::GetTypeCache() {
    return *typeCache;
}

bool VCL::CompilerContext::HasTypeCache() {
    return typeCache != nullptr;
}

void VCL::CompilerContext::CreateTypeCache() {
    typeCache = llvm::makeIntrusiveRefCnt<TypeCache>();
}

void VCL::CompilerContext::CopyTypeCache(CompilerContext& other) {
    typeCache = other.typeCache;
}

VCL::ModuleCache& VCL::CompilerContext::GetModuleCache() {
    return *moduleCache;
}

bool VCL::CompilerContext::HasModuleCache() {
    return moduleCache != nullptr;
}

void VCL::CompilerContext::CreateModuleCache() {
    moduleCache = llvm::makeIntrusiveRefCnt<ModuleCache>();
}

void VCL::CompilerContext::CopyModuleCache(CompilerContext& other) {
    moduleCache = other.moduleCache;
}

llvm::orc::ThreadSafeContext& VCL::CompilerContext::GetLLVMContext() {
    return llvmContext;
}

bool VCL::CompilerContext::HasLLVMContext() {
    return llvmContext.withContextDo([](llvm::LLVMContext* ctx){ return ctx != nullptr; });
}

void VCL::CompilerContext::CreateLLVMContext() {
    llvmContext = llvm::orc::ThreadSafeContext{ std::make_unique<llvm::LLVMContext>() };
}

std::shared_ptr<VCL::CompilerInstance> VCL::CompilerContext::CreateInstance() {
    return std::make_shared<CompilerInstance>(*this);
}