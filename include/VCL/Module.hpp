#pragma once

#include <VCL/ModuleUtils.hpp>
#include <VCL/AST.hpp>
#include <VCL/Logger.hpp>

#include <memory>
#include <filesystem>


namespace VCL {
    class ModuleContext;

    class Module {
    public:
        Module() = delete;
        Module(std::unique_ptr<ModuleContext> context);
        ~Module();

        void BindProgram(std::unique_ptr<ASTProgram> program);

        void Emit(ModuleDebugInformationSettings settings = {});

        void Verify(ModuleVerifierSettings settings = {});

        void Optimize(ModuleOptimizerSettings settings = {});

        std::string Dump();

    private:
        friend class ExecutionSession;

        std::unique_ptr<ModuleContext> context;
        std::unique_ptr<ASTProgram> program;
    };
};