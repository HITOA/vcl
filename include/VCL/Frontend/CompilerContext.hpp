#pragma once

#include <VCL/Frontend/CompilerInvocation.hpp>

#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>

#include <memory>


namespace VCL {
    class DiagnosticsEngine;
    class DiagnosticReporter;
    class DiagnosticConsumer;
    class SourceManager;
    class IdentifierTable;
    class Target;
    class CompilerInstance;

    class CompilerContext {
    public:
        CompilerContext();
        CompilerContext(std::shared_ptr<CompilerInvocation> invocation);
        CompilerContext(const CompilerContext& other) = delete;
        CompilerContext(CompilerContext&& other) = delete;
        ~CompilerContext();
        
        CompilerContext& operator=(const CompilerContext& other) = delete;
        CompilerContext& operator=(CompilerContext&& other) = delete;

        CompilerInvocation& GetInvocation() { return *invocation; }

        DiagnosticsEngine& GetDiagnosticsEngine();
        DiagnosticReporter& GetDiagnosticReporter();
        bool HasDiagnosticEngine();
        bool CreateDiagnosticEngine();
        
        SourceManager& GetSourceManager();
        bool HasSourceManager();
        bool CreateSourceManager();

        IdentifierTable& GetIdentifierTable();
        bool HasIdentifierTable();
        bool CreateIdentifierTable();

        Target& GetTarget();
        bool HasTarget();
        bool CreateTarget();

        llvm::orc::ThreadSafeContext& GetLLVMContext();
        bool HasLLVMContext();
        bool CreateLLVMContext();

        std::shared_ptr<CompilerInstance> CreateInstance();
    
    private:
        std::shared_ptr<CompilerInvocation> invocation;
        llvm::IntrusiveRefCntPtr<DiagnosticsEngine> diagnosticsEngine;
        llvm::IntrusiveRefCntPtr<DiagnosticReporter> diagnosticReporter;
        llvm::IntrusiveRefCntPtr<SourceManager> sourceManager;
        llvm::IntrusiveRefCntPtr<IdentifierTable> identifierTable;
        llvm::IntrusiveRefCntPtr<Target> target;
        llvm::orc::ThreadSafeContext llvmContext;
    };

}