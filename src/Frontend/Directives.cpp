#include <VCL/Frontend/Directives.hpp>

#include <VCL/Core/Diagnostic.hpp>
#include <VCL/Core/Format.hpp>
#include <VCL/Core/SourceManager.hpp>
#include <VCL/AST/Decl.hpp>
#include <VCL/Sema/Sema.hpp>
#include <VCL/Frontend/CompilerContext.hpp>
#include <VCL/Frontend/ModuleCache.hpp>
#include <VCL/Frontend/FrontendActions.hpp>

#include <llvm/Support/Path.h>
#include <llvm/ADT/SmallString.h>


bool VCL::ImportDirective::OnSema(Sema& sema, DirectiveDecl* decl) {
    if (decl->GetArgs().size() > 1) {
        sema.GetDiagnosticReporter().Error(Diagnostic::DirectiveError, "import directive only take one argument")
            .AddHint(DiagnosticHint{ decl->GetSourceRange() })
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return false;
    }
    if (decl->GetArgs().size() < 1) {
        sema.GetDiagnosticReporter().Error(Diagnostic::DirectiveError, "import directive is missing its import path")
            .AddHint(DiagnosticHint{ decl->GetSourceRange() })
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return false;
    }
    if (decl->GetArgs()[0]->GetConstantValueClass() != ConstantValue::ConstantStringClass) {
        sema.GetDiagnosticReporter().Error(Diagnostic::DirectiveError, "import directive only take an import path")
            .AddHint(DiagnosticHint{ decl->GetSourceRange() })
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return false;
    }

    llvm::SmallString<128> fullImportPath{ importRootDirectory };

    ConstantString* constString = (ConstantString*)decl->GetArgs()[0];
    std::string importPath = ParseStringLiteral(constString->GetString());
    llvm::sys::path::append(fullImportPath, importPath);
    llvm::StringRef stem = llvm::sys::path::stem(fullImportPath);
    IdentifierInfo* identifierInfo = sema.GetIdentifierTable().Get(stem);

    Source* source = compilerContext.GetSourceManager().LoadFromDisk(fullImportPath);

    if (!source) {
        sema.GetDiagnosticReporter().Error(Diagnostic::DirectiveError, "import directive couldn't read source")
            .AddHint(DiagnosticHint{ decl->GetSourceRange() })
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return false;
    }

    Module* m = compilerContext.GetModuleCache().Get(source);
    bool r = true;
    if (!m) {
        EmitLLVMAction act{};
        act.SetRunOptimization(false);
        std::shared_ptr<CompilerInstance> ci = compilerContext.CreateInstance();
        ci->BeginSource(source);
        r = ci->ExecuteAction(act);
        ci->EndSource();
        if (r)
            m = compilerContext.GetModuleCache().Add(source, ci, act.MoveModule());
    }

    if (!r || !m) {
        sema.GetDiagnosticReporter().Error(Diagnostic::DirectiveError, "failed to import " + source->GetBufferIdentifier().str())
            .AddHint(DiagnosticHint{ decl->GetSourceRange() })
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return false;
    }

    return sema.ImportModule(m, identifierInfo);
}