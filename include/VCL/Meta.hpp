#pragma once

#include <VCL/AST.hpp>
#include <VCL/Lexer.hpp>

#include <unordered_map>


namespace VCL {
    class Parser;
    
    class MetaState {
        
    };

    class DirectiveHandler {
    public:
        virtual ~DirectiveHandler() = default;

        virtual std::string GetDirectiveName() = 0;
        virtual std::unique_ptr<ASTDirective> Parse(Lexer& lexer, Parser* parser) = 0;
    };

    class DirectiveRegistry {
    public:
        DirectiveRegistry() = default;
        ~DirectiveRegistry() = default;
        
        void RegisterDirective(std::shared_ptr<DirectiveHandler> handler);
        std::shared_ptr<DirectiveHandler> GetDirective(std::string name);

        void RegisterDefaultDirectives();

        static std::shared_ptr<DirectiveRegistry> Create();

    private:
        std::unordered_map<std::string, std::shared_ptr<DirectiveHandler>> handlers{};
    };
}