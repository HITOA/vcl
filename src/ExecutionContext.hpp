#pragma once

#include <llvm/IR/LLVMContext.h>

#include <llvm/ExecutionEngine/JITSymbol.h>
#include <llvm/ExecutionEngine/JITEventListener.h>
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
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/Support/Debug.h>

#include <functional>
#include <filesystem>


namespace VCL {

    class ExecutionContext {
    public:
        ExecutionContext();
        ~ExecutionContext();

        void AddModule(llvm::orc::ThreadSafeModule module, llvm::orc::ResourceTrackerSP rt = nullptr);

        llvm::orc::ExecutorSymbolDef Lookup(std::string_view name);

        void DefineExternSymbolPtr(std::string_view name, void* buffer);

        void SetDumpObject(std::filesystem::path directory, std::string_view identifier);

        void EnableDebugInformation();
        void DisableDebugInformation();

        /**
         * @brief Get the underlying thread safe LLVMContext.
         */
        llvm::orc::ThreadSafeContext& GetTSContext();

    private:
        llvm::Expected<std::unique_ptr<llvm::MemoryBuffer>> DumpObject(std::unique_ptr<llvm::MemoryBuffer> buf);

    private:
        llvm::orc::ThreadSafeContext context;

        std::unique_ptr<llvm::orc::ExecutionSession> session;

        std::unique_ptr<llvm::orc::MangleAndInterner> mangle;
        std::unique_ptr<llvm::DataLayout> layout;

        std::unique_ptr<llvm::orc::RTDyldObjectLinkingLayer> linkingLayer;
        std::unique_ptr<llvm::orc::ObjectTransformLayer::TransformFunction> dumpObjectTransformFunction;
        std::unique_ptr<llvm::orc::ObjectTransformLayer> dumpObjectLayer;
        std::unique_ptr<llvm::orc::IRCompileLayer> compileLayer;
        
        llvm::orc::JITDylib* main;

        llvm::JITEventListener* gdbListener;

        bool dumpObject;
        std::filesystem::path dumpObjectDirectory;
        std::string_view dumpObjectIdentifier;
    };

}