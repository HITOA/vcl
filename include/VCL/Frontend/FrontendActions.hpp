#pragma once

#include <VCL/Frontend/FrontendAction.hpp>
#include <VCL/CodeGen/CodeGenAction.hpp>


namespace VCL {

    class ASTConsumer;

    class ParseSyntaxOnlyAction : public FrontendAction {
    public:
        bool Execute() override;

        /** Sees each top-level declaration as it's parsed (and may adjust it, as vcl-graph does). */
        inline void SetASTConsumer(ASTConsumer* consumer) { this->consumer = consumer; }

    private:
        ASTConsumer* consumer = nullptr;
    };
    
    class EmitLLVMAction : public FrontendAction, public CodeGenAction {
    public:
        bool Execute() override;

        inline bool GetRunOptimization() const { return runOptimization; }
        inline void SetRunOptimization(bool runOptimization) { this->runOptimization = runOptimization; }

    private:
        bool runOptimization = true;
    };

}