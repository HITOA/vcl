#pragma once

#include <VCL/Frontend/FrontendAction.hpp>
#include <VCL/CodeGen/CodeGenAction.hpp>


namespace VCL {

    class ParseSyntaxOnlyAction : public FrontendAction {
    public:
        bool Execute() override;
    };

    class EmitLLVMAction : public FrontendAction, public CodeGenAction {
    public:
        bool Execute() override;
    };

}