#pragma once

#include <VCL/Module.hpp>
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

        void Emit();

        void Optimize();

        std::string Dump();

    private:
        std::unique_ptr<ModuleContext> context;
        std::unique_ptr<ASTProgram> program;
    };
};