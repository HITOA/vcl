#include <VCL/Core/TargetOptions.hpp>

#include <llvm/Target/TargetMachine.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/MC/MCSubtargetInfo.h>
#include <llvm/TargetParser/SubtargetFeature.h>
#include <llvm/ADT/StringSet.h>


VCL::TargetOptions::TargetOptions() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();

    triple = llvm::sys::getProcessTriple();
    cpu = llvm::sys::getHostCPUName();
    auto& cpuFeatures = llvm::sys::getHostCPUFeatures();
    llvm::SubtargetFeatures features{};
    for (auto& feature : cpuFeatures)
        features.AddFeature(feature.first(), feature.second);

    this->features = features.getString();
}