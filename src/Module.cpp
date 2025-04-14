#include <VCL/Module.hpp>

#include "ModuleContext.hpp"
#include "ModuleBuilder.hpp"
#include "ModuleOptimizer.hpp"
#include "Intrinsics.hpp"

#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_os_ostream.h>

#include <sstream>


VCL::Module::Module(std::unique_ptr<ModuleContext> context) : context{ std::move(context) } {
    Intrinsics::Register(this->context.get());
}

VCL::Module::~Module() {

}

void VCL::Module::BindProgram(std::unique_ptr<ASTProgram> program) {
    this->program = std::move(program);
}

void VCL::Module::Emit() {
    ModuleBuilder builder{ context.get() };
    program->Accept(&builder);
    std::stringstream sstream{};
    llvm::raw_os_ostream llvmStream{ sstream };
    bool brokenDebugInfo;
    if (llvm::verifyModule(*context->GetTSModule().getModuleUnlocked(), &llvmStream, &brokenDebugInfo))
        throw std::runtime_error{ sstream.str() };
    if (brokenDebugInfo && context->GetLogger())
        context->GetLogger()->Warning("{}", sstream.str());
}

void VCL::Module::Optimize() {
    ModuleOptimizerSettings settings{};
    settings.enableInliner = false;
    std::unique_ptr<ModuleOptimizer> optimizer = ModuleOptimizer::Create(settings);
    optimizer->Run(context.get());
}

std::string VCL::Module::Dump() {
    std::string str;
    llvm::raw_string_ostream output{ str };
    context->GetTSModule().getModuleUnlocked()->print(output, nullptr);
    return str;
}