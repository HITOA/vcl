#include <VCL/Core/Target.hpp>

#include <llvm/MC/TargetRegistry.h>
#include <llvm/MC/MCSubtargetInfo.h>
#include <llvm/TargetParser/SubtargetFeature.h>
#include <llvm/ADT/StringSet.h>


VCL::Target::Target() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();

    auto triple = llvm::Triple{ llvm::sys::getProcessTriple() };
    std::string error;
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(triple, error);

    llvm::StringRef cpuName = llvm::sys::getHostCPUName();
    auto& cpuFeatures = llvm::sys::getHostCPUFeatures();
    llvm::SubtargetFeatures features{};
    for (auto& feature : cpuFeatures)
        features.AddFeature(feature.first(), feature.second);

    llvm::TargetOptions options{};

    llvm::TargetMachine* targetMachine = 
        target->createTargetMachine(triple, cpuName, features.getString(), options, std::nullopt, std::nullopt, llvm::CodeGenOptLevel::Aggressive);
    tm = std::unique_ptr<llvm::TargetMachine>{ targetMachine };

    CacheTargetMetadata();
}

VCL::Target::Target(std::unique_ptr<llvm::TargetMachine> tm) : tm{ std::move(tm) } {
    CacheTargetMetadata();
}

VCL::Target::Target(TargetOptions& options) {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    
    auto triple = llvm::Triple{ options.GetTriple() };
    std::string error;
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(triple, error);

    llvm::TargetOptions targetOptions{};
    llvm::TargetMachine* targetMachine = 
        target->createTargetMachine(triple, options.GetCPU(), options.GetFeatures(), targetOptions, std::nullopt, std::nullopt, llvm::CodeGenOptLevel::Aggressive);
    tm = std::unique_ptr<llvm::TargetMachine>{ targetMachine };

    CacheTargetMetadata();
}

void VCL::Target::CacheTargetMetadata() {
    llvm::SubtargetFeatures features{ tm->getTargetFeatureString() };

    llvm::StringSet enabledFeatures{};

    for (auto& feature : features.getFeatures()) {
        bool enabled = feature[0] == '+';
        llvm::StringRef name{ feature.data() + 1, feature.size() - 1 };
        if (enabled)
            enabledFeatures.insert(name);
    }
    
    if (enabledFeatures.count("avx512f")) {
        vectorWidthInByte = 64;
    } else if (enabledFeatures.count("avx2")) {
        vectorWidthInByte = 32;
    } else if (enabledFeatures.count("sse2")) {
        vectorWidthInByte = 16;
    } else {
        vectorWidthInByte = 8;
    }
}