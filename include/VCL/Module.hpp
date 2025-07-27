#pragma once

#include <VCL/ModuleInfo.hpp>
#include <VCL/AST.hpp>
#include <VCL/Logger.hpp>

#include <memory>
#include <filesystem>


namespace VCL {
    class ModuleContext;

    struct ModuleDebugInformationSettings {
        bool generateDebugInformation = false;
        bool optimized = false;
    };

    struct ModuleOptimizerSettings {
        bool enableLoopUnrolling = true;
        bool enableInliner = true;
        bool enableVectorizer = true;
    };

    struct ModuleVerifierSettings {
        bool brokenDebugInformationAsError = false;
        bool enableSelectRecursionCheck = true;
        bool selectRecursionAsError = false;
    };

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

        std::shared_ptr<ModuleInfo> GetModuleInfo();

    private:
        friend class ExecutionSession;

        std::unique_ptr<ModuleContext> context;
        std::unique_ptr<ASTProgram> program;
    };
};