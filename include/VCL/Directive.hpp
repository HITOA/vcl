#pragma once

#include <VCL/Meta.hpp>
#include <VCL/Error.hpp>
#include <VCL/Source.hpp>

#include <expected>
#include <unordered_set>
#include <unordered_map>


namespace VCL {

    class ImportDirective : public DirectiveHandler {
    public:
        std::string GetDirectiveName() override;
        std::unique_ptr<ASTDirective> Parse(Lexer& lexer, Parser* parser) override;
        void Run(ModuleContext* context, ASTDirective* directive, ASTVisitor* visitor) override;

    protected:
        virtual std::expected<std::shared_ptr<Source>, Error> LoadSource(const std::string& path);
        
    public:
        class ASTImportDirective : public ASTDirective {
        public:
            ASTImportDirective(const std::string& importPath) : importPath{ importPath } {};

            std::string GetName() const override { return "import"; }

        public:
            std::string importPath;
        };

        class ImportDirectiveMetaComponent : public MetaComponent {
        public:
            ImportDirectiveMetaComponent() = default;
            ~ImportDirectiveMetaComponent() = default;

            bool TrackImport(const std::string& path);

        private:
            std::unordered_set<std::string> imports{};
        };
    };

    class DefineDirective : public DirectiveHandler {
    public:
        std::string GetDirectiveName() override;
        std::unique_ptr<ASTDirective> Parse(Lexer& lexer, Parser* parser) override;
        void Run(ModuleContext* context, ASTDirective* directive, ASTVisitor* visitor) override;

    public:
        class ASTDefineDirective : public ASTDirective {
        public:
            ASTDefineDirective(const std::string& name, std::unique_ptr<ASTLiteralExpression> expression) : 
                name{ name }, expression{ std::move(expression) } {};

            std::string GetName() const override { return "define"; }

        public:
            std::string name;
            std::unique_ptr<ASTLiteralExpression> expression;
        };

        class DefineDirectiveMetaComponent : public MetaComponent {
        public:
            DefineDirectiveMetaComponent() = default;
            ~DefineDirectiveMetaComponent() = default;

            bool Defined(const std::string& name);
            ASTLiteralExpression* GetDefine(const std::string& name);
            bool AddDefine(const std::string& name, std::unique_ptr<ASTLiteralExpression> value);

            bool AddDefineFlag(const std::string& name);
            bool AddDefineInt(const std::string& name, int value);
            bool AddDefineFloat(const std::string& name, float value);

        public:
            std::unordered_map<std::string, std::unique_ptr<ASTLiteralExpression>> defines{};
        };
    };

    class ConditionalDirective : public DirectiveHandler {
    public:
        std::string GetDirectiveName() override;
        std::unique_ptr<ASTDirective> Parse(Lexer& lexer, Parser* parser) override;
        void Run(ModuleContext* context, ASTDirective* directive, ASTVisitor* visitor) override;

    public:
        class ASTConditionalDirective : public ASTDirective {
        public:
            ASTConditionalDirective(std::unique_ptr<ASTExpression> expression, std::unique_ptr<ASTStatement> thenStmt,
                std::unique_ptr<ASTStatement> elseStmt) : 
                expression{ std::move(expression) }, thenStmt{ std::move(thenStmt) }, elseStmt{ std::move(elseStmt) } {};

            std::string GetName() const override { return "if"; }

        public:
            std::unique_ptr<ASTExpression> expression;
            std::unique_ptr<ASTStatement> thenStmt;
            std::unique_ptr<ASTStatement> elseStmt;
        };
    };

}