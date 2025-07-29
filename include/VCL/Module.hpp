#pragma once

#include <VCL/ModuleInfo.hpp>
#include <VCL/AST.hpp>
#include <VCL/Logger.hpp>
#include <VCL/Meta.hpp>

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

        /**
         * @brief Bind a program to be used for this module
         */
        void BindProgram(std::unique_ptr<ASTProgram> program);

        /**
         * @brief Emit IR code of this module.
         */
        void Emit(ModuleDebugInformationSettings settings = {});

        /**
         * @brief Check for error in the emitted IR.
         */
        void Verify(ModuleVerifierSettings settings = {});

        /**
         * @brief Apply given set of optimization to the emitted IR.
         */
        void Optimize(ModuleOptimizerSettings settings = {});

        /**
         * @brief Dump the emitted IR as a readable string.
         */
        std::string Dump();

        /**
         * @brief Get information for this module.
         */
        std::shared_ptr<ModuleInfo> GetModuleInfo();

        /**
         * @brief Set the logger that will be used for this module only.
         */
        void SetLogger(std::shared_ptr<Logger> logger);

        /**
         * @brief Set the directive registry this module will use for compilation directive.
         */
        void SetDirectiveRegistry(std::shared_ptr<DirectiveRegistry> registry);

        /**
         * @brief Set the MetaState class instance this module will use for compiler directive.
         */
        void SetMetaState(std::shared_ptr<MetaState> state);

    private:
        friend class ExecutionSession;

        std::unique_ptr<ModuleContext> context;
        std::unique_ptr<ASTProgram> program;
    };
};