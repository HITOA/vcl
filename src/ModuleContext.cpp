#include "ModuleContext.hpp"

#include <VCL/NativeTarget.hpp>


VCL::ModuleContext::ModuleContext(std::string_view name, llvm::orc::ThreadSafeContext context, std::shared_ptr<Logger> logger) :
    module{ std::make_unique<llvm::Module>(name, *context.getContext()), context }, irBuilder{ *context.getContext() },
    diBuilder{ *module.getModuleUnlocked() }, sm{}, logger{ logger } {
    
    diBuiltinFile = diBuilder.createFile("builtin", "/");

    basicTypes.floatDIType = diBuilder.createBasicType("float", 32, llvm::dwarf::DW_ATE_float);
    basicTypes.intDIType = diBuilder.createBasicType("int", 32, llvm::dwarf::DW_ATE_signed);
    basicTypes.boolDIType = diBuilder.createBasicType("bool", 32, llvm::dwarf::DW_ATE_boolean);
    basicTypes.voidDIType = diBuilder.createBasicType("void", 0, llvm::dwarf::DW_ATE_address);

    uint32_t vectorElementWidth = NativeTarget::GetInstance()->GetMaxVectorElementWidth();
    uint32_t vectorBitWidth = NativeTarget::GetInstance()->GetMaxVectorByteWidth() * 8;

    auto* subrange = diBuilder.getOrCreateSubrange(0, vectorElementWidth);
    auto subscripts = diBuilder.getOrCreateArray(subrange);

    llvm::DICompositeType* vfloatBaseType = diBuilder.createVectorType(vectorBitWidth, vectorBitWidth, basicTypes.floatDIType, subscripts);
    llvm::DICompositeType* vintBaseType = diBuilder.createVectorType(vectorBitWidth, vectorBitWidth, basicTypes.intDIType, subscripts);
    llvm::DICompositeType* vboolBaseType = diBuilder.createVectorType(vectorBitWidth, vectorBitWidth, basicTypes.boolDIType, subscripts);
    basicTypes.vfloatDIType = diBuilder.createTypedef(vfloatBaseType, "vfloat", diBuiltinFile, 0, nullptr);
    basicTypes.vintDIType = diBuilder.createTypedef(vintBaseType, "vint", diBuiltinFile, 0, nullptr);
    basicTypes.vboolDIType = diBuilder.createTypedef(vboolBaseType, "vbool", diBuiltinFile, 0, nullptr);

    info = std::make_shared<ModuleInfo>();
}

VCL::ModuleContext::~ModuleContext() {

}

llvm::orc::ThreadSafeContext VCL::ModuleContext::GetTSContext() {
    return module.getContext();
}

llvm::orc::ThreadSafeModule& VCL::ModuleContext::GetTSModule() {
    return module;
}

llvm::IRBuilder<>& VCL::ModuleContext::GetIRBuilder() {
    return irBuilder;
}

llvm::DIBuilder& VCL::ModuleContext::GetDIBuilder() {
    return diBuilder;
}

VCL::ScopeManager& VCL::ModuleContext::GetScopeManager() {
    return sm;
}

std::shared_ptr<VCL::Logger> VCL::ModuleContext::GetLogger() {
    return logger;
}

VCL::DebugInformationBasicType* VCL::ModuleContext::GetDIBasicTypes() {
    return &basicTypes;
}

llvm::DIFile* VCL::ModuleContext::GetDIBuiltinFile() {
    return diBuiltinFile;
}

std::shared_ptr<VCL::ModuleInfo> VCL::ModuleContext::GetModuleInfo() {
    return info;
}