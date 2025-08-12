#pragma once

#include <VCL/Logger.hpp>
#include <VCL/AST.hpp>
#include <VCL/Module.hpp>
#include <VCL/Storage.hpp>

#include <memory>


namespace VCL {
    class ExecutionContext;

    class ExecutionSession {
    public:
        ExecutionSession();
        ~ExecutionSession();

        /**
         * @brief Create a VCL Module for compilation
         */
        std::unique_ptr<Module> CreateModule(std::unique_ptr<ASTProgram> program);

        /**
         * @brief Submit a VCL Module for compilation
         */
        void SubmitModule(std::unique_ptr<Module> module);

        /**
         * @brief Reset the current session
         */
        void Clear();

        /**
         * @brief Define a symbol by its address and its name.
         */
        void DefineExternSymbolPtr(std::string_view name, void* buffer);

        /**
         * @brief Define a symbol with its storage class.
         */
        void DefineExternSymbolStorage(std::string_view name, Storage* storage);

        /**
         * @brief Lookup compiled vcl function's address by name and returns it.
         */
        void* Lookup(std::string_view name);

        /**
         * @brief Dump compiled object to given directory.
         */
        void SetDumpObject(std::filesystem::path directory, std::string_view identifier);

        /**
         * @brief Enable or disable debug information for this session.
         */
        void SetDebugInformation(bool enabled);

        /**
         * @brief Set the logger class instance the parser will use to send error warning info and debug messages. 
         */
        void SetLogger(std::shared_ptr<Logger> logger);

        static std::unique_ptr<ExecutionSession> Create(std::shared_ptr<Logger> logger = nullptr);

    private:
        std::shared_ptr<Logger> logger;
        std::unique_ptr<ExecutionContext> context;
    };

}