#include <VCL/CodeGen/CodeGenModule.hpp>

#include <VCL/AST/Expr.hpp>


VCL::CodeGenModule::CodeGenModule(llvm::orc::ThreadSafeModule& module, ASTContext& ast, DiagnosticReporter& diagnosticReporter, Target& target)
    : module{ module }, astContext{ ast }, diagnosticReporter{ diagnosticReporter }, target{ target }, cgt{ *this } {
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
        case Decl::FunctionDeclClass: return EmitFunctionDecl((FunctionDecl*)decl);
        default: return true;
    }
}

bool VCL::CodeGenModule::EmitGlobalVarDecl(VarDecl* decl) {
    llvm::Type* type = cgt.ConvertType(decl->GetValueType());
    if (!type)
        return false;

    llvm::Constant* initializerValue = llvm::Constant::getNullValue(type);
    if (Expr* initializer = decl->GetInitializer(); initializer != nullptr) {
        ConstantValue* initValue = initializer->GetConstantValue();
        if (!initValue) {
            diagnosticReporter.Error(Diagnostic::InternalError)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return false;
        }
        initializerValue = GenerateConstantValue(initValue);
        if (!initializerValue)
            return false;
        // A bit of a cheat in the overall compiler but it work and its just simpler
        Type* v = decl->GetValueType().GetType();
        if (v->GetTypeClass() == Type::TemplateSpecializationTypeClass)
            v = ((TemplateSpecializationType*)v)->GetInstantiatedType();
        if (v->GetTypeClass() == Type::VectorTypeClass)
            initializerValue = llvm::ConstantDataVector::getSplat(GetTarget().GetVectorWidthInElement(), initializerValue);
    }

    llvm::Constant* entry = GetLLVMModule().getOrInsertGlobal(decl->GetIdentifierInfo()->GetName(), type);

    llvm::GlobalVariable* gv = (llvm::GlobalVariable*)entry;
    
    llvm::GlobalVariable::LinkageTypes linkageType = llvm::GlobalVariable::LinkageTypes::CommonLinkage;
    bool isConstant = false;

    if ((!decl->HasOutAttribute() && decl->HasInAttribute()) || decl->GetValueType().HasQualifier(Qualifier::Const))
        isConstant = true;
    if (decl->HasInAttribute() || decl->HasOutAttribute())
        linkageType = llvm::GlobalVariable::LinkageTypes::ExternalLinkage;

    if (!decl->HasInAttribute())
        gv->setInitializer(initializerValue);
    
    gv->setConstant(isConstant);
    gv->setLinkage(linkageType);
    if (linkageType != llvm::GlobalVariable::LinkageTypes::ExternalLinkage)
        gv->setDSOLocal(true);

    globals.insert(std::make_pair(decl, gv));

    return true;
}

bool VCL::CodeGenModule::EmitFunctionDecl(FunctionDecl* decl) {
    CodeGenFunction cgf{ *this };
    llvm::Function* function = cgf.Generate(decl);
    if (function == nullptr)
        return false;
    globals.insert(std::make_pair(decl, function));
    return true;
}

llvm::GlobalValue* VCL::CodeGenModule::GetGlobalDeclValue(Decl* decl) {
    if (globals.count(decl))
        return globals.at(decl);
    return nullptr;
}