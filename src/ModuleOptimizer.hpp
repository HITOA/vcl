#pragma once

#include <VCL/Module.hpp>

#include "ModuleContext.hpp"

#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Scalar/Reassociate.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include <llvm/Transforms/IPO/Inliner.h>
#include <llvm/Transforms/Vectorize/SLPVectorizer.h>
#include <llvm/Transforms/Vectorize/LoopVectorize.h>
#include <llvm/Transforms/Scalar/LoopUnrollPass.h>
#include <llvm/Transforms/Scalar/IndVarSimplify.h>
#include <llvm/Transforms/Scalar/LICM.h>
#include <llvm/Transforms/Utils/Mem2Reg.h>
#include <llvm/Passes/PassBuilder.h>

#include <memory>


namespace VCL {

    class ModuleOptimizer {
    public:
        ModuleOptimizer() = delete;
        ModuleOptimizer(ModuleOptimizerSettings settings);
        ~ModuleOptimizer();

        void Run(ModuleContext* context);

        static std::unique_ptr<ModuleOptimizer> Create(ModuleOptimizerSettings settings);

    private:
        //Analysis Manager
        std::unique_ptr<llvm::LoopAnalysisManager> lam;
        std::unique_ptr<llvm::FunctionAnalysisManager> fam;
        std::unique_ptr<llvm::CGSCCAnalysisManager> cgam;
        std::unique_ptr<llvm::ModuleAnalysisManager> mam;

        //Pass Builder
        std::unique_ptr<llvm::PassBuilder> pb;

        //Pass Manager
        std::unique_ptr<llvm::LoopPassManager> lpm;
        std::unique_ptr<llvm::FunctionPassManager> fpm;
        std::unique_ptr<llvm::CGSCCPassManager> cgpm;
        std::unique_ptr<llvm::ModulePassManager> mpm;
    };

}