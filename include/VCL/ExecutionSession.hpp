#pragma once

#include <VCL/Logger.hpp>
#include <VCL/AST.hpp>
#include <VCL/Module.hpp>

#include <memory>


namespace VCL {
    class ExecutionContext;

    class ExecutionSession {
    public:
        ExecutionSession();
        ~ExecutionSession();

        std::unique_ptr<Module> CreateModule(std::unique_ptr<ASTProgram> program);

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