#pragma once

#include "Scope.hpp"

#include <VCL/Logger.hpp>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>

#include <string_view>


namespace VCL {

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
         * @brief Get the underlying LLVM IRBuilder<> module.
         */
        llvm::IRBuilder<>& GetIRBuilder();
        
        /**
         * @brief Get the ScopeManager of this module.
         */
        ScopeManager& GetScopeManager();

        /**
         * @brief Get the logger of this module.
         */
        std::shared_ptr<Logger> GetLogger();

    private:
        llvm::orc::ThreadSafeModule module;
        llvm::IRBuilder<> irBuilder;
        ScopeManager sm;
        std::shared_ptr<Logger> logger;
    };

}