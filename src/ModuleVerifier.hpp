#pragma once

#include <VCL/Module.hpp>

#include "ModuleContext.hpp"

#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Analysis/CallGraph.h>

#include <sstream>
#include <memory>


namespace VCL {

    /**
     * @brief Verify module for problem in the ir including things like infinite recursion
     */
    class ModuleVerifier {
    public:
        ModuleVerifier() = delete;
        ModuleVerifier(ModuleVerifierSettings settings);
        ~ModuleVerifier();

        void Verify(ModuleContext* context);

        static std::unique_ptr<ModuleVerifier> Create(ModuleVerifierSettings settings);

    private:
        void VerifySelectRecursion(ModuleContext* context, llvm::CallGraph& graph);
        bool IsExpressionRecursive(llvm::Value* value, const llvm::Function* function, llvm::CallGraph& graph);
        bool IsRecursive(const llvm::Function* function, llvm::CallGraphNode* node, int depth = 10);

    private:
        ModuleVerifierSettings settings;
    };

}