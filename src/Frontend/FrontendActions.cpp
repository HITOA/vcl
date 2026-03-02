#include <VCL/Frontend/FrontendActions.hpp>

#include <VCL/Frontend/CompilerContext.hpp>
#include <VCL/Frontend/CompilerInstance.hpp>
#include <VCL/Core/Source.hpp>
#include <VCL/Lex/Lexer.hpp>
#include <VCL/Lex/TokenStream.hpp>
#include <VCL/Sema/Sema.hpp>
#include <VCL/Parse/Parser.hpp>
#include <VCL/CodeGen/CodeGenModule.hpp>
#include <VCL/CodeGen/Optimizer.hpp>

#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>

#include <assert.h>

bool VCL::ParseSyntaxOnlyAction::Execute() {
    assert(instance->HasSource() && "missing source");
    assert(instance->GetCompilerContext().HasDiagnosticEngine() && "missing diagnostic engine");
    assert(instance->GetCompilerContext().HasIdentifierTable() && "missing identifier table");
    assert(instance->GetCompilerContext().HasAttributeTable() && "missing attribute table");
    assert(instance->GetCompilerContext().HasDirectiveRegistry() && "missing directive registry");

    assert(!instance->HasASTContext() && "ast context already present");
    assert(!instance->HasExportSymbolTable() && "exported symbol table already present");
    assert(!instance->HasImportModuleTable() && "imported module table already present");

    instance->CreateASTContext();
    instance->CreateExportSymbolTable();
    instance->CreateImportModuleTable();
    instance->CreateDefineTable();

    Lexer lexer{ instance->GetSource()->GetBufferRef(), 
        instance->GetCompilerContext().GetDiagnosticReporter(), 
        instance->GetCompilerContext().GetIdentifierTable() };
    TokenStream stream{ lexer };
    Sema sema{ instance->GetASTContext(),
        instance->GetCompilerContext().GetDiagnosticReporter(),
        instance->GetCompilerContext().GetIdentifierTable(),
        instance->GetCompilerContext().GetDirectiveRegistry(),
        instance->GetExportSymbolTable(),
        instance->GetImportModuleTable(),
        instance->GetDefineTable() };
    Parser parser{ stream, sema, instance->GetCompilerContext().GetAttributeTable() };
    
    return parser.Parse();
}

bool VCL::EmitLLVMAction::Execute() {
    assert(instance->HasSource() && "missing source");
    assert(instance->GetCompilerContext().HasDiagnosticEngine() && "missing diagnostic engine");
    assert(instance->GetCompilerContext().HasIdentifierTable() && "missing identifier table");
    assert(instance->GetCompilerContext().HasAttributeTable() && "missing attribute table");
    assert(instance->GetCompilerContext().HasDirectiveRegistry() && "missing directive registry");
    assert(instance->GetCompilerContext().HasLLVMContext() && "missing LLVM Context");
    assert(instance->GetCompilerContext().HasTarget() && "missing target");

    assert(!instance->HasASTContext() && "ast context already present");
    assert(!instance->HasExportSymbolTable() && "exported symbol table already present");
    assert(!instance->HasImportModuleTable() && "imported module table already present");

    instance->CreateASTContext();
    instance->CreateExportSymbolTable();
    instance->CreateImportModuleTable();
    instance->CreateDefineTable();

    Lexer lexer{ instance->GetSource()->GetBufferRef(), 
        instance->GetCompilerContext().GetDiagnosticReporter(), 
        instance->GetCompilerContext().GetIdentifierTable() };
    TokenStream stream{ lexer };
    Sema sema{ instance->GetASTContext(),
        instance->GetCompilerContext().GetDiagnosticReporter(),
        instance->GetCompilerContext().GetIdentifierTable(),
        instance->GetCompilerContext().GetDirectiveRegistry(),
        instance->GetExportSymbolTable(),
        instance->GetImportModuleTable(),
        instance->GetDefineTable() };
    Parser parser{ stream, sema, instance->GetCompilerContext().GetAttributeTable() };
    
    if (!parser.Parse())
        return false;
    
    module = llvm::orc::ThreadSafeModule{ 
        instance->GetCompilerContext().GetLLVMContext().withContextDo([](llvm::LLVMContext* context){
            return std::make_unique<llvm::Module>("module", *context);
        }),
        instance->GetCompilerContext().GetLLVMContext() };

    return module.withModuleDo([this](llvm::Module& module){
        CodeGenModule cgm{
            module, 
            instance->GetASTContext(), 
            instance->GetCompilerContext().GetDiagnosticReporter(),
            instance->GetCompilerContext().GetTarget(),
            instance->GetImportModuleTable(),
            instance->GetCompilerContext().GetAttributeTable(),
            instance->GetCompilerContext().GetIdentifierTable() };
        if (!cgm.Emit())
            return false;
        if (runOptimization) {
            Optimizer optimizer{};
            return optimizer.Optimize(cgm);
        } else {
            return true;
        }
    });
}