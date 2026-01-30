#include <VCL/CodeGen/Optimizer.hpp>

#include <VCL/CodeGen/CodeGenModule.hpp>

#include <llvm/Passes/PassBuilder.h>


bool VCL::Optimizer::Optimize(CodeGenModule& cgm) {
    llvm::LoopAnalysisManager lam{};
    llvm::FunctionAnalysisManager fam{};
    llvm::CGSCCAnalysisManager cgam{};
    llvm::ModuleAnalysisManager mam{};

    llvm::PassBuilder pb{};

    pb.registerModuleAnalyses(mam);
    pb.registerCGSCCAnalyses(cgam);
    pb.registerFunctionAnalyses(fam);
    pb.registerLoopAnalyses(lam);

    pb.crossRegisterProxies(lam, fam, cgam, mam);

    llvm::ModulePassManager mpm = pb.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O3);

    if (!cgm.LinkNow())
        return false;

    mpm.run(cgm.GetLLVMModule(), mam);
    return true;
}