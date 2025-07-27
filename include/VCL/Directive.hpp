#pragma once

#include <VCL/Meta.hpp>


namespace VCL {

    class ImportDirective : public DirectiveHandler {
    public:
        std::string GetDirectiveName() override;
        std::unique_ptr<ASTDirective> Parse(Lexer& lexer, Parser* parser) override;
        
    public:
        class ASTImportDirective : public ASTDirective {
        public:
            ASTImportDirective(const std::string& importPath) : importPath{ importPath } {};

            std::string GetName() const override { return "import"; }

        private:
            std::string importPath;
        };
    };

    class DefineDirective : public DirectiveHandler {
    public:
        std::string GetDirectiveName() override;
        std::unique_ptr<ASTDirective> Parse(Lexer& lexer, Parser* parser) override;

    public:
        class ASTDefineDirective : public ASTDirective {
        public:
            ASTDefineDirective(const std::string& name, std::unique_ptr<ASTLiteralExpression> expression) : 
                name{ name }, expression{ std::move(expression) } {};

            std::string GetName() const override { return "define"; }

        private:
            std::string name;
            std::unique_ptr<ASTLiteralExpression> expression;
        };
    };

    class MacroDirective : public DirectiveHandler {
    public:
        std::string GetDirectiveName() override;
        std::unique_ptr<ASTDirective> Parse(Lexer& lexer, Parser* parser) override;

    public:
        class ASTMacroDirective : public ASTDirective {
        public:
            ASTMacroDirective(const std::string& name, std::unique_ptr<ASTCompoundStatement> statements) : 
                name{ name }, statements{ std::move(statements) } {};

            std::string GetName() const override { return "macro"; }

        private:
            std::string name;
            std::unique_ptr<ASTCompoundStatement> statements;
        };
    };

    /*class ConditionalDirective : public DirectiveHandler {
    public:
        std::string GetDirectiveName() override;
        std::unique_ptr<ASTDirective> Parse(Lexer& lexer) override;
    };*/

}