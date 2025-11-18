#include <VCL/Frontend/FrontendActions.hpp>

#include <VCL/Frontend/CompilerContext.hpp>
#include <VCL/Frontend/CompilerInstance.hpp>
#include <VCL/Core/Source.hpp>
#include <VCL/Lex/Lexer.hpp>
#include <VCL/Lex/TokenStream.hpp>
#include <VCL/Sema/Sema.hpp>
#include <VCL/Parse/Parser.hpp>
#include <VCL/CodeGen/CodeGenModule.hpp>

#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>


bool VCL::ParseSyntaxOnlyAction::Execute() {
    if (!instance->HasSource())
        return false;
    if (instance->HasASTContext())
        return false;

    if (!instance->CreateASTContext())
        return false;

    Lexer lexer{ instance->GetSource()->GetBufferRef(), 
        instance->GetCompilerContext().GetDiagnosticReporter(), 
        instance->GetCompilerContext().GetIdentifierTable() };
    TokenStream stream{ lexer };
    Sema sema{ instance->GetASTContext(),
        instance->GetCompilerContext().GetDiagnosticReporter(),
        instance->GetCompilerContext().GetIdentifierTable() };
    Parser parser{ stream, sema };
    
    return parser.Parse();
}

bool VCL::EmitLLVMAction::Execute() {
    if (!instance->HasSource())
        return false;
    if (instance->HasASTContext())
        return false;

    if (!instance->CreateASTContext())
        return false;

    Lexer lexer{ instance->GetSource()->GetBufferRef(), 
        instance->GetCompilerContext().GetDiagnosticReporter(), 
        instance->GetCompilerContext().GetIdentifierTable() };
    TokenStream stream{ lexer };
    Sema sema{ instance->GetASTContext(),
        instance->GetCompilerContext().GetDiagnosticReporter(),
        instance->GetCompilerContext().GetIdentifierTable() };
    Parser parser{ stream, sema };
    
    if (!parser.Parse())
        return false;
    
    module = llvm::orc::ThreadSafeModule{ 
        instance->GetCompilerContext().GetLLVMContext().withContextDo([](llvm::LLVMContext* context){
            return std::make_unique<llvm::Module>("module", *context);
        }),
        instance->GetCompilerContext().GetLLVMContext() };

    return module.withModuleDo([this](llvm::Module& module){
        CodeGenModule cgm{ module, 
            instance->GetASTContext(), 
            instance->GetCompilerContext().GetDiagnosticReporter(),
            instance->GetCompilerContext().GetTarget() };
        return cgm.Emit();
    });
}