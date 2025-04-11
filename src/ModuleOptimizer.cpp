#include "ModuleOptimizer.hpp"



VCL::ModuleOptimizer::ModuleOptimizer(ModuleOptimizerSettings settings) {
    lam = std::make_unique<llvm::LoopAnalysisManager>();
    fam = std::make_unique<llvm::FunctionAnalysisManager>();
    cgam = std::make_unique<llvm::CGSCCAnalysisManager>();
    mam = std::make_unique<llvm::ModuleAnalysisManager>();

    pb = std::make_unique<llvm::PassBuilder>();
    pb->registerModuleAnalyses(*mam);
    pb->registerCGSCCAnalyses(*cgam);
    pb->registerFunctionAnalyses(*fam);
    pb->registerLoopAnalyses(*lam);
    pb->crossRegisterProxies(*lam, *fam, *cgam, *mam);

    lpm = std::make_unique<llvm::LoopPassManager>();
    fpm = std::make_unique<llvm::FunctionPassManager>();
    cgpm = std::make_unique<llvm::CGSCCPassManager>();
    mpm = std::make_unique<llvm::ModulePassManager>();

    lpm->addPass(llvm::IndVarSimplifyPass{});
    //lpm->addPass(llvm::LICMPass{ llvm::LICMOptions{} });

    fpm->addPass(llvm::createFunctionToLoopPassAdaptor(std::move(*lpm)));

    fpm->addPass(llvm::PromotePass{});
    fpm->addPass(llvm::InstCombinePass{});
    if (settings.enableVectorizer)
        fpm->addPass(llvm::LoopVectorizePass{});
    if (settings.enableLoopUnrolling)
        fpm->addPass(llvm::LoopUnrollPass{});
    fpm->addPass(llvm::ReassociatePass{});
    fpm->addPass(llvm::GVNPass{});
    fpm->addPass(llvm::SimplifyCFGPass{});
    if (settings.enableVectorizer)
        fpm->addPass(llvm::SLPVectorizerPass{});

    cgpm->addPass(llvm::createCGSCCToFunctionPassAdaptor(std::move(*fpm)));

    if (settings.enableInliner)
        cgpm->addPass(llvm::InlinerPass{});

    mpm->addPass(llvm::createModuleToPostOrderCGSCCPassAdaptor(std::move(*cgpm)));
}

VCL::ModuleOptimizer::~ModuleOptimizer() {

}

void VCL::ModuleOptimizer::Run(ModuleContext* context) {
    mpm->run(context->GetModule(), *mam);
}

std::unique_ptr<VCL::ModuleOptimizer> VCL::ModuleOptimizer::Create(ModuleOptimizerSettings settings) {
    return std::make_unique<ModuleOptimizer>(settings);
}