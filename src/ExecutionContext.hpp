#pragma once

#include <llvm/IR/LLVMContext.h>


namespace VCL {

    class ExecutionContext {
    public:
        ExecutionContext();
        ~ExecutionContext();

        /**
         * @brief Get the underlying LLVMContext.
         */
        llvm::LLVMContext& GetContext();

    private:
        llvm::LLVMContext context;
    };

}