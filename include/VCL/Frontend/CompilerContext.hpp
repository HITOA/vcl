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
    class AttributeTable;
    class DirectiveRegistry;
    class Target;
    class TypeCache;
    class CompilerInstance;
    class ModuleCache;

    class CompilerContext {
    public:
        CompilerContext();
        CompilerContext(std::shared_ptr<CompilerInvocation> invocation);
        CompilerContext(const CompilerContext& other) = delete;
        CompilerContext(CompilerContext&& other) = delete;
        ~CompilerContext();
        
        CompilerContext& operator=(const CompilerContext& other) = delete;
        CompilerContext& operator=(CompilerContext&& other) = delete;

        inline std::shared_ptr<CompilerInvocation> GetInvocation() { return invocation; }

        DiagnosticsEngine& GetDiagnosticsEngine();
        DiagnosticReporter& GetDiagnosticReporter();
        bool HasDiagnosticEngine();
        void CreateDiagnosticEngine();
        void CopyDiagnosticEngine(CompilerContext& other);
        
        SourceManager& GetSourceManager();
        bool HasSourceManager();
        void CreateSourceManager();
        void CopySourceManager(CompilerContext& other);

        IdentifierTable& GetIdentifierTable();
        bool HasIdentifierTable();
        void CreateIdentifierTable();
        void CopyIdentifierTable(CompilerContext& other);

        AttributeTable& GetAttributeTable();
        bool HasAttributeTable();
        void CreateAttributeTable();
        void CopyAttributeTable(CompilerContext& other);

        DirectiveRegistry& GetDirectiveRegistry();
        bool HasDirectiveRegistry();
        void CreateDirectiveRegistry();
        void CopyDirectiveRegistry(CompilerContext& other);

        Target& GetTarget();
        bool HasTarget();
        void CreateTarget();
        void CopyTarget(CompilerContext& other);

        TypeCache& GetTypeCache();
        bool HasTypeCache();
        void CreateTypeCache();
        void CopyTypeCache(CompilerContext& other);

        ModuleCache& GetModuleCache();
        bool HasModuleCache();
        void CreateModuleCache();
        void CopyModuleCache(CompilerContext& other);

        llvm::orc::ThreadSafeContext& GetLLVMContext();
        bool HasLLVMContext();
        void CreateLLVMContext();

        std::shared_ptr<CompilerInstance> CreateInstance();
    
    private:
        std::shared_ptr<CompilerInvocation> invocation;
        llvm::IntrusiveRefCntPtr<DiagnosticsEngine> diagnosticsEngine;
        llvm::IntrusiveRefCntPtr<DiagnosticReporter> diagnosticReporter;
        llvm::IntrusiveRefCntPtr<SourceManager> sourceManager;
        llvm::IntrusiveRefCntPtr<IdentifierTable> identifierTable;
        llvm::IntrusiveRefCntPtr<AttributeTable> attributeTable;
        llvm::IntrusiveRefCntPtr<DirectiveRegistry> directiveRegistry;
        llvm::IntrusiveRefCntPtr<Target> target;
        llvm::IntrusiveRefCntPtr<TypeCache> typeCache;
        llvm::IntrusiveRefCntPtr<ModuleCache> moduleCache;
        llvm::orc::ThreadSafeContext llvmContext;
    };

}