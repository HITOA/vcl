#pragma once

#include "Scope.hpp"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>


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
        ModuleContext(llvm::LLVMContext* context);
        ~ModuleContext();

        /**
         * @brief Get the LLVMContext that own this module.
         */
        llvm::LLVMContext& GetContext() const;

        /**
         * @brief Get the underlying LLVM Module.
         */
        llvm::Module& GetModule() const;

        /**
         * @brief Get the underlying LLVM IRBuilder<> module.
         */
        llvm::IRBuilder<>& GetIRBuilder() const;
        
        /**
         * @brief Get the ScopeManager of this module.
         */
        ScopeManager& GetScopeManager() const;

    private:
        llvm::LLVMContext* context;
        llvm::Module module;
        llvm::IRBuilder<> irBuilder;
        ScopeManager sm;
    };

}