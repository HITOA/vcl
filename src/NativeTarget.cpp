#include <VCL/NativeTarget.hpp>

#include <llvm/Support/TargetSelect.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/ADT/StringMap.h>


VCL::NativeTarget::NativeTarget() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
}

VCL::NativeTarget::~NativeTarget() {

}

uint32_t VCL::NativeTarget::GetMaxVectorByteWidth() {
    static uint32_t maxVectorWidthInBytes = 0;
    if (maxVectorWidthInBytes == 0) {
        llvm::StringMap<bool> features = llvm::sys::getHostCPUFeatures();
        if (features["avx512f"]) maxVectorWidthInBytes = 64;
        else if (features["avx2"]) maxVectorWidthInBytes = 32;
        else if (features["sse2"]) maxVectorWidthInBytes = 16;
        else maxVectorWidthInBytes = 8;
    }
    return maxVectorWidthInBytes;
}

uint32_t VCL::NativeTarget::GetMaxVectorElementWidth() {
    return GetMaxVectorByteWidth() / sizeof(float);
}

std::shared_ptr<VCL::NativeTarget> VCL::NativeTarget::GetInstance() {
    static std::shared_ptr<NativeTarget> instance = nullptr;
    if (!instance)
        instance = std::make_shared<NativeTarget>();
    return instance;
}