#include <VCL/Module.hpp>

#include "ModuleContext.hpp"
#include "ModuleBuilder.hpp"
#include "ModuleOptimizer.hpp"
#include "ModuleVerifier.hpp"
#include "Intrinsics.hpp"


VCL::Module::Module(std::unique_ptr<ModuleContext> context) : context{ std::move(context) } {
    Intrinsics::Register(this->context.get());
}

VCL::Module::~Module() {

}

void VCL::Module::BindProgram(std::unique_ptr<ASTProgram> program) {
    this->program = std::move(program);
}

void VCL::Module::Emit(ModuleDebugInformationSettings settings) {
    ModuleBuilder builder{ context.get() };
    builder.SetDISettings(settings);
    program->Accept(&builder);
    context->GetDIBuilder().finalize();
}

void VCL::Module::Verify(ModuleVerifierSettings settings) {
    std::unique_ptr<ModuleVerifier> verifier = ModuleVerifier::Create(settings);
    verifier->Verify(context.get());
}

void VCL::Module::Optimize(ModuleOptimizerSettings settings) {
    std::unique_ptr<ModuleOptimizer> optimizer = ModuleOptimizer::Create(settings);
    optimizer->Run(context.get());
}

std::string VCL::Module::Dump() {
    std::string str;
    llvm::raw_string_ostream output{ str };
    context->GetTSModule().getModuleUnlocked()->print(output, nullptr);
    return str;
}