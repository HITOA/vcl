#include <VCL/Frontend/CompilerInstance.hpp>

#include <VCL/AST/ASTContext.hpp>
#include <VCL/Frontend/CompilerContext.hpp>
#include <VCL/Frontend/FrontendAction.hpp>
#include <VCL/Sema/SymbolTable.hpp>
#include <VCL/Sema/ModuleTable.hpp>
#include <VCL/Sema/DefineTable.hpp>
#include <VCL/CodeGen/Mangler.hpp>

#include <assert.h>


VCL::CompilerInstance::CompilerInstance(CompilerContext& compilerCtx) : 
        compilerCtx{ compilerCtx }, source{ nullptr }, astCtx{ nullptr }, importedModules{} {

}

VCL::CompilerInstance::~CompilerInstance() {
    
}

VCL::CompilerContext& VCL::CompilerInstance::GetCompilerContext() {
    return compilerCtx;
}

VCL::ASTContext& VCL::CompilerInstance::GetASTContext() {
    return *astCtx;
}

bool VCL::CompilerInstance::HasASTContext() {
    return astCtx != nullptr;
}

void VCL::CompilerInstance::CreateASTContext() {
    assert(compilerCtx.HasTypeCache() && "missing type cache");
    astCtx = llvm::makeIntrusiveRefCnt<ASTContext>(compilerCtx.GetTypeCache());
}

VCL::SymbolTable& VCL::CompilerInstance::GetExportSymbolTable() {
    return *exportedSymbols;
}

bool VCL::CompilerInstance::HasExportSymbolTable() {
    return exportedSymbols != nullptr;
}

void VCL::CompilerInstance::CreateExportSymbolTable() {
    exportedSymbols = llvm::makeIntrusiveRefCnt<SymbolTable>();
}

VCL::ModuleTable& VCL::CompilerInstance::GetImportModuleTable() {
    return *importedModules;
}

bool VCL::CompilerInstance::HasImportModuleTable() {
    return importedModules != nullptr;
}

void VCL::CompilerInstance::CreateImportModuleTable() {
    importedModules = llvm::makeIntrusiveRefCnt<ModuleTable>();
}

VCL::DefineTable& VCL::CompilerInstance::GetDefineTable() {
    return *definedValues;
}

bool VCL::CompilerInstance::HasDefineTable() {
    return definedValues != nullptr;
}

void VCL::CompilerInstance::CreateDefineTable() {
    definedValues = llvm::makeIntrusiveRefCnt<DefineTable>();
}

bool VCL::CompilerInstance::BeginSource(Source* source) {
    if (this->source != nullptr)
        return false;
    this->source = source;
    return true;
}

bool VCL::CompilerInstance::EndSource() {
    this->source = nullptr;
    return true;
}

bool VCL::CompilerInstance::HasSource() {
    return source != nullptr;
}

VCL::Source* VCL::CompilerInstance::GetSource() {
    return source;
}

bool VCL::CompilerInstance::ExecuteAction(FrontendAction& action) {
    if (!action.Prepare(this))
        return false;
    return action.Execute();
}

std::optional<std::string> VCL::CompilerInstance::GetMangledSymbolName(llvm::StringRef name) {
    if (!HasASTContext())
        return {};
    if (!compilerCtx.HasIdentifierTable())
        return {};

    IdentifierInfo* identifierInfo = compilerCtx.GetIdentifierTable().Get(name);

    TranslationUnitDecl* tu = astCtx->GetTranslationUnitDecl();
    for (auto it = tu->Begin(); it != tu->End(); ++it) {
        if (!it->IsNamedDecl())
            continue;
        NamedDecl* namedDecl = (NamedDecl*)it.Get();
        if (namedDecl->GetIdentifierInfo() != identifierInfo)
            continue;
        switch (namedDecl->GetDeclClass()) {
            case Decl::FunctionDeclClass: return Mangler::MangleFunctionDecl(GetASTContext(), (FunctionDecl*)namedDecl);
            case Decl::VarDeclClass: return Mangler::MangleVarDecl(GetASTContext(), (VarDecl*)namedDecl);
            default: return name.str();
        }
    }

    return {};
}