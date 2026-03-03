#include <VCL/CodeGen/CodeGenModule.hpp>

#include <VCL/AST/Expr.hpp>
#include <VCL/AST/Decl.hpp>
#include <VCL/AST/DeclTemplate.hpp>
#include <VCL/CodeGen/Mangler.hpp>

#include <llvm/Linker/Linker.h>
#include <llvm/Transforms/Utils/Cloning.h>


VCL::CodeGenModule::CodeGenModule(llvm::Module& module, ASTContext& ast, DiagnosticReporter& diagnosticReporter, Target& target, 
        ModuleTable& importedModules, AttributeTable& attributeTable, IdentifierTable& identifierTable)
    : module{ module }, astContext{ ast }, diagnosticReporter{ diagnosticReporter }, 
        target{ target }, cgt{ *this }, importedModules{ importedModules }, attributeTable{ attributeTable }, identifierTable{ identifierTable } {
    module.setDataLayout(target.GetTargetMachine()->createDataLayout());
    module.setTargetTriple(target.GetTargetMachine()->getTargetTriple());
}

bool VCL::CodeGenModule::LinkNow() {
    llvm::Linker linker{ module };

    for (auto module : importedModules) {
        std::unique_ptr<llvm::Module> clonedModule = module.second->GetModule().withModuleDo([this](llvm::Module& module){
            return llvm::CloneModule(module);
        });
        if (linker.linkInModule(std::move(clonedModule))) {
            diagnosticReporter.Error(Diagnostic::InternalError)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return false;
        }
    }

    return true;
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
    switch (decl->GetDeclClass()) {
        case Decl::VarDeclClass: return EmitGlobalVarDecl((VarDecl*)decl);
        case Decl::FunctionDeclClass: return EmitFunctionDecl((FunctionDecl*)decl);
        case Decl::TemplateDeclClass: return EmitTemplateDecl((TemplateDecl*)decl);
        default: return true;
    }
}

bool VCL::CodeGenModule::EmitGlobalVarDecl(VarDecl* decl, bool imported) {
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

    llvm::GlobalVariable::LinkageTypes linkageType = llvm::GlobalVariable::LinkageTypes::InternalLinkage;
    bool isConstant = false;

    if ((!decl->HasOutAttribute() && decl->HasInAttribute()) || decl->GetValueType().HasQualifier(Qualifier::Const))
        isConstant = true;
    if (decl->HasInAttribute() || decl->HasOutAttribute())
        linkageType = llvm::GlobalVariable::LinkageTypes::ExternalLinkage;
    else if (imported)
        linkageType = llvm::GlobalVariable::LinkageTypes::LinkOnceAnyLinkage;

    std::string globalName = decl->GetIdentifierInfo()->GetName().str();
    if (linkageType != llvm::GlobalVariable::LinkageTypes::ExternalLinkage) {
        if (imported) {
            Module* module = GetImportedDeclModule(decl);
            globalName = Mangler::MangleVarDecl(module->GetCompilerInstance()->GetASTContext(), decl);
        } else {
            globalName = Mangler::MangleVarDecl(astContext, decl);
        }
    }
    
    llvm::Constant* entry = GetLLVMModule().getOrInsertGlobal(globalName, type);

    llvm::GlobalVariable* gv = (llvm::GlobalVariable*)entry;

    if (!decl->HasInAttribute() && !imported)
        gv->setInitializer(initializerValue);
    
    gv->setConstant(isConstant);
    gv->setLinkage(linkageType);
    if (linkageType != llvm::GlobalVariable::LinkageTypes::ExternalLinkage)
        gv->setDSOLocal(true);

    globals.insert(std::make_pair(decl, gv));

    return true;
}

bool VCL::CodeGenModule::EmitFunctionDecl(FunctionDecl* decl, bool imported) {
    CodeGenFunction cgf{ *this };
    llvm::Function* function = cgf.Generate(decl, imported);
    if (function == nullptr)
        return false;
    auto insertResult = globals.insert(std::make_pair(decl, function));
    return insertResult.second;
}

bool VCL::CodeGenModule::EmitTemplateDecl(TemplateDecl* decl) {
    for (auto it = decl->Begin(); it != decl->End(); ++it) {
        if (it->GetDeclClass() != Decl::TemplateSpecializationDeclClass)
            continue;

        TemplateSpecializationDecl* specializationDecl = (TemplateSpecializationDecl*)it.Get();
        NamedDecl* specializedDecl = specializationDecl->GetNamedDecl();
        switch (specializedDecl->GetDeclClass()) {
            case Decl::FunctionDeclClass: {
                if (((FunctionDecl*)specializedDecl)->HasFunctionFlag(FunctionDecl::IsIntrinsic))
                    return true;
                if (!EmitFunctionDecl((FunctionDecl*)specializedDecl))
                    return false;
            }
        }
    }
    return true;
}

bool VCL::CodeGenModule::IsDeclImported(Decl* decl) {
    return GetImportedDeclModule(decl) != nullptr;
}

VCL::Module* VCL::CodeGenModule::GetImportedDeclModule(Decl* decl) {
    if (!decl->IsNamedDecl())
        return nullptr;

    for (auto& module : importedModules) {
        for (auto& importedDeclPair : module.second->GetCompilerInstance()->GetExportSymbolTable()) {
            Decl* importedDecl = importedDeclPair.second;
            if (importedDecl == decl)
                return module.second;
            if (importedDecl->IsTemplateDecl()) {
                TemplateDecl* templateDecl = (TemplateDecl*)importedDecl;
                for (auto it = templateDecl->Begin(); it != templateDecl->End(); ++it) {
                    if (it->GetDeclClass() != Decl::TemplateSpecializationDeclClass)
                        continue;

                    TemplateSpecializationDecl* specializationDecl = (TemplateSpecializationDecl*)it.Get();
                    NamedDecl* specializedDecl = specializationDecl->GetNamedDecl();
                    if (decl == specializedDecl)
                        return module.second;
                }
            }
        }
    }

    return nullptr;
}

bool VCL::CodeGenModule::EmitImportedDecl(Decl* decl) {
    switch (decl->GetDeclClass()) {
        case Decl::VarDeclClass: return EmitGlobalVarDecl((VarDecl*)decl, true);
        case Decl::FunctionDeclClass: return EmitFunctionDecl((FunctionDecl*)decl, true);
        default: return false;
    }
}

llvm::GlobalValue* VCL::CodeGenModule::GetGlobalDeclValue(Decl* decl) {
    if (globals.count(decl))
        return globals.at(decl);

    if (IsDeclImported(decl)) {
        if (!EmitImportedDecl(decl))
            return nullptr;
        return globals.at(decl);
    }

    return nullptr;
}