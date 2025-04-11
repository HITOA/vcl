#pragma once

#include "Scope.hpp"

#include <VCL/Logger.hpp>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>

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
        ModuleContext(std::string_view name, llvm::LLVMContext* context, std::shared_ptr<Logger> logger);
        ~ModuleContext();

        /**
         * @brief Get the LLVMContext that own this module.
         */
        llvm::LLVMContext& GetContext();

        /**
         * @brief Get the underlying LLVM Module.
         */
        llvm::Module& GetModule();

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
        llvm::LLVMContext* context;
        llvm::Module module;
        llvm::IRBuilder<> irBuilder;
        ScopeManager sm;
        std::shared_ptr<Logger> logger;
    };

}