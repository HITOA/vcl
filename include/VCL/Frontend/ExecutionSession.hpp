#pragma once
        
#include <llvm/ExecutionEngine/JITSymbol.h>
#include <llvm/ExecutionEngine/JITEventListener.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/Core.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/Orc/ExecutorProcessControl.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/Orc/ObjectTransformLayer.h>
#include <llvm/ExecutionEngine/Orc/DebugUtils.h>
#include <llvm/ExecutionEngine/Orc/Shared/ExecutorSymbolDef.h>
#include <llvm/ExecutionEngine/Orc/SelfExecutorProcessControl.h>
#include <llvm/ExecutionEngine/Orc/AbsoluteSymbols.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/Support/Debug.h>

#include <memory>


namespace VCL {

    class ExecutionSession {
    public:
        ExecutionSession();
        ExecutionSession(const ExecutionSession& other) = delete;
        ExecutionSession(ExecutionSession&& other);
        ~ExecutionSession();

        ExecutionSession& operator=(const ExecutionSession& other) = delete;
        ExecutionSession& operator=(ExecutionSession&& other) = default;

        inline llvm::Error ConsumeLastError() { return std::move(lastError); }

        bool SubmitModule(llvm::orc::ThreadSafeModule&& module);

        void* Lookup(llvm::StringRef name);
        bool DefineSymbolPtr(llvm::StringRef name, void* ptr);

        void EnableGDBListener();
        void DisableGDBListener();

    private:
        std::unique_ptr<llvm::orc::ExecutionSession> session;
        std::unique_ptr<llvm::DataLayout> layout;
        std::unique_ptr<llvm::orc::MangleAndInterner> mangle;
        std::unique_ptr<llvm::orc::RTDyldObjectLinkingLayer> linkingLayer;
        std::unique_ptr<llvm::orc::IRCompileLayer> compileLayer;
        llvm::orc::JITDylib* main;
        llvm::JITEventListener* gdbListener;
        llvm::Error lastError;
    };

}