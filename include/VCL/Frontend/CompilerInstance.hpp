#pragma once

#include <llvm/ADT/IntrusiveRefCntPtr.h>


namespace VCL {
    class CompilerContext;
    class Source;
    class ASTContext;
    class FrontendAction;

    class CompilerInstance {
    public:
        CompilerInstance() = delete;
        CompilerInstance(CompilerContext& compilerCtx);
        CompilerInstance(const CompilerInstance& other) = delete;
        CompilerInstance(CompilerInstance&& other) = delete;
        ~CompilerInstance();

        CompilerInstance& operator=(const CompilerInstance& other) = delete;
        CompilerInstance& operator=(CompilerInstance&& other) = delete;

        CompilerContext& GetCompilerContext();

        ASTContext& GetASTContext();
        bool HasASTContext();
        bool CreateASTContext();

        bool BeginSource(Source* source);
        bool EndSource();
        bool HasSource();
        Source* GetSource();
        
        bool ExecuteAction(FrontendAction& action);

    private:
        CompilerContext& compilerCtx;
        Source* source;
        llvm::IntrusiveRefCntPtr<ASTContext> astCtx;
    };

}