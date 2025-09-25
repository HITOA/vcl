#include <VCL/CodeGen/CodeGenModule.hpp>



VCL::CodeGenModule::CodeGenModule(llvm::orc::ThreadSafeModule& module, ASTContext& ast, CompilerContext& cc, Target& target)
    : module{ module }, astContext{ ast }, cc{ cc }, target{ target }, cgt{ *this } {
    module.getModuleUnlocked()->setDataLayout(target.GetTargetMachine()->createDataLayout());
    module.getModuleUnlocked()->setTargetTriple(target.GetTargetMachine()->getTargetTriple().getTriple());
}

bool VCL::CodeGenModule::Emit() {
    TranslationUnitDecl* tu = astContext.GetTranslationUnitDecl();
    for (auto it = tu->Begin(); it != tu->End(); ++it) {
        if (!EmitTopLevelDecl(it.Get()))
            return false;
    }
    return true;
}

bool VCL::CodeGenModule::EmitTopLevelDecl(Decl* decl) {
    if (decl->IsTemplateDecl())
        return true;
    switch (decl->GetDeclClass()) {
        case Decl::VarDeclClass: return EmitGlobalVarDecl((VarDecl*)decl);
        default: return true;
    }
}

bool VCL::CodeGenModule::EmitGlobalVarDecl(VarDecl* decl) {
    llvm::Type* type = cgt.ConvertType(decl->GetType());
    if (!type)
        return false;

    llvm::Constant* initializerValue = llvm::Constant::getNullValue(type);
    llvm::Constant* entry = GetLLVMModule().getOrInsertGlobal(decl->GetIdentifierInfo()->GetName(), type);

    llvm::GlobalVariable* gv = (llvm::GlobalVariable*)entry;
    
    llvm::GlobalVariable::LinkageTypes linkageType = llvm::GlobalVariable::LinkageTypes::CommonLinkage;
    bool isConstant = false;

    if ((!decl->HasOutAttribute() && decl->HasInAttribute()) || decl->GetType().HasQualifier(Qualifier::Const))
        isConstant = true;
    if (decl->HasInAttribute() || decl->HasOutAttribute())
        linkageType = llvm::GlobalVariable::LinkageTypes::ExternalLinkage;
    
    gv->setConstant(isConstant);
    gv->setLinkage(linkageType);
    if (linkageType != llvm::GlobalVariable::LinkageTypes::ExternalLinkage)
        gv->setDSOLocal(true);

    return true;
}