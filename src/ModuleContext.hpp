#pragma once

#include "Scope.hpp"

#include <VCL/Logger.hpp>
#include <VCL/ModuleInfo.hpp>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>

#include <string_view>


namespace VCL {

    struct DebugInformationBasicType {
        llvm::DIBasicType* floatDIType;
        llvm::DIBasicType* intDIType;
        llvm::DIBasicType* boolDIType;
        llvm::DIBasicType* voidDIType;
        llvm::DIDerivedType* vfloatDIType;
        llvm::DIDerivedType* vintDIType;
        llvm::DIDerivedType* vboolDIType;
    };

    /**
     * @brief This is the main class representing a JIT Module.
     *
     * It hold all the context data to build and compile the module into
     * propper machine code by the JIT compiler. It also hold a ptr
     * to the LLVMContext owned by the JITContext class it's a part of.
     */
    class ModuleContext {
    public:
        ModuleContext() = delete;
        ModuleContext(std::string_view name, llvm::orc::ThreadSafeContext context, std::shared_ptr<Logger> logger);
        ~ModuleContext();

        /**
         * @brief Get the thread safe LLVMContext that own this module.
         */
        llvm::orc::ThreadSafeContext GetTSContext();

        /**
         * @brief Get the underlying thread safe LLVM Module.
         */
        llvm::orc::ThreadSafeModule& GetTSModule();

        /**
         * @brief Get the underlying LLVM IRBuilder<>.
         */
        llvm::IRBuilder<>& GetIRBuilder();

        /**
         * @brief Get the underlyind LLVM DIBuilder.
         */
        llvm::DIBuilder& GetDIBuilder();
        
        /**
         * @brief Get the ScopeManager of this module.
         */
        ScopeManager& GetScopeManager();

        /**
         * @brief Get the logger of this module.
         */
        std::shared_ptr<Logger> GetLogger();

        /**
         * @brief Get debug information basic type struct.
         */
        DebugInformationBasicType* GetDIBasicTypes();

        /**
         * @brief Get debug information builtin file.
         */
        llvm::DIFile* GetDIBuiltinFile();

        /**
         * @brief Get module information.
         */
        std::shared_ptr<ModuleInfo> GetModuleInfo();

    private:
        llvm::orc::ThreadSafeModule module;
        llvm::IRBuilder<> irBuilder;
        llvm::DIBuilder diBuilder;
        llvm::DIFile* diBuiltinFile;
        ScopeManager sm;
        std::shared_ptr<Logger> logger;
        std::shared_ptr<ModuleInfo> info;
        DebugInformationBasicType basicTypes;
    };

}