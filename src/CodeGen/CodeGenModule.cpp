#include <VCL/CodeGen/CodeGenModule.hpp>

#include <VCL/AST/Expr.hpp>
#include <VCL/AST/Decl.hpp>
#include <VCL/AST/DeclTemplate.hpp>
#include <VCL/CodeGen/Mangler.hpp>

#include <llvm/Linker/Linker.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Constants.h>


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

    if (llvm::verifyModule(module, &llvm::errs())) {
        diagnosticReporter.Error(Diagnostic::InternalError)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return false;
    }

    return true;
}

bool VCL::CodeGenModule::Emit(bool verifyModule) {
    return Emit(astContext.GetTranslationUnitDecl(), verifyModule);
}

bool VCL::CodeGenModule::Emit(TranslationUnitDecl* tu, bool verifyModule) {
    for (auto it = tu->Begin(); it != tu->End(); ++it) {
        if (!EmitTopLevelDecl(it.Get()))
            return false;
    }

    if (verifyModule) {
        if (llvm::verifyModule(module, &llvm::errs())) {
            diagnosticReporter.Error(Diagnostic::InternalError)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return false;
        }
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

    llvm::GlobalVariable::LinkageTypes linkageType = decl->IsExported() ? 
            llvm::GlobalVariable::LinkageTypes::ExternalLinkage : llvm::GlobalVariable::LinkageTypes::InternalLinkage;
    bool isConstant = false;

    if ((!decl->HasOutAttribute() && decl->HasInAttribute()) || decl->GetValueType().HasQualifier(Qualifier::Const))
        isConstant = true;
    if (decl->HasInAttribute() || decl->HasOutAttribute())
        linkageType = llvm::GlobalVariable::LinkageTypes::ExternalLinkage;
    else if (imported)
        linkageType = llvm::GlobalVariable::LinkageTypes::ExternalLinkage;

    // Named after the module that declares it, so an importer refers to the same symbol.
    ASTContext& owningContext = imported ? GetImportedDeclModule(decl)->GetCompilerInstance()->GetASTContext() : astContext;
    std::string globalName = Mangler::MangleVarDecl(owningContext, decl);
    
    llvm::Constant* entry = GetLLVMModule().getOrInsertGlobal(globalName, type);

    llvm::GlobalVariable* gv = (llvm::GlobalVariable*)entry;

    // Every AST emitted into one llvm::Module must have its own mangling prefix; two different
    // declarations landing on one symbol would silently share state.
    if (!imported && !decl->HasInAttribute() && !decl->HasOutAttribute()) {
        if (Decl* existing = GetSymbolDecl(gv); existing != nullptr && existing != decl) {
            diagnosticReporter.Error(Diagnostic::SymbolNameCollision, globalName)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return false;
        }
        SetSymbolDecl(gv, decl);
    }

    if (!decl->HasInAttribute() && !imported)
        gv->setInitializer(initializerValue);
    
    gv->setConstant(isConstant);
    gv->setLinkage(linkageType);
    if (linkageType != llvm::GlobalVariable::LinkageTypes::ExternalLinkage)
        gv->setDSOLocal(true);

    gv->setAlignment(llvm::Align{ target.GetVectorWidthInByte() });

    globals.insert(std::make_pair(decl, gv));

    return true;
}

bool VCL::CodeGenModule::EmitFunctionDecl(FunctionDecl* decl, bool imported) {
    CodeGenFunction cgf{ *this };
    llvm::Function* function = cgf.Generate(decl, imported);
    if (function == nullptr)
        return false;
    auto insertResult = globals.insert(std::make_pair(decl, function));
    // Already there when it was emitted on demand (GetGlobalDeclValue) before being reached here.
    return insertResult.second || insertResult.first->second == function;
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
                    continue;
                if (!EmitFunctionDecl((FunctionDecl*)specializedDecl))
                    return false;
                break;
            }
            default:
                break;
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

    // A specialization instantiated in this compilation but not stored with its template (see
    // Sema::GetInstantiationContext): emit it the first time it's referenced.
    if (decl->GetDeclClass() == Decl::FunctionDeclClass && ((FunctionDecl*)decl)->HasFunctionFlag(FunctionDecl::IsTemplateSpecialization)) {
        if (!EmitFunctionDecl((FunctionDecl*)decl))
            return nullptr;
        return globals.at(decl);
    }

    return nullptr;
}
void VCL::CodeGenModule::SetSymbolDecl(llvm::GlobalObject* symbol, Decl* decl) {
    llvm::LLVMContext& context = symbol->getContext();
    llvm::Constant* address = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), (uint64_t)(uintptr_t)decl);
    symbol->setMetadata("vcl.decl", llvm::MDNode::get(context, { llvm::ConstantAsMetadata::get(address) }));
}

VCL::Decl* VCL::CodeGenModule::GetSymbolDecl(llvm::GlobalObject* symbol) {
    llvm::MDNode* node = symbol->getMetadata("vcl.decl");
    if (!node || node->getNumOperands() != 1)
        return nullptr;
    auto* address = llvm::mdconst::dyn_extract<llvm::ConstantInt>(node->getOperand(0));
    return address ? (Decl*)(uintptr_t)address->getZExtValue() : nullptr;
}
