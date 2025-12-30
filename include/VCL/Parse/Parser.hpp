#pragma once

#include <VCL/Lex/TokenStream.hpp>
#include <VCL/AST/ASTContext.hpp>
#include <VCL/AST/Decl.hpp>
#include <VCL/AST/Stmt.hpp>
#include <VCL/AST/Expr.hpp>
#include <VCL/AST/Template.hpp>
#include <VCL/AST/DeclTemplate.hpp>

#include <optional>


namespace VCL {
    class Sema;

    class Parser {
    public:
        enum class ParserFlag : uint32_t {
            None = 0x0,
            OnTemplateArgument = 0x1
        };

    public:
        Parser() = delete;
        Parser(TokenStream& stream, Sema& sema);
        Parser(const Parser& other) = delete;
        Parser(Parser&& other) = delete;
        ~Parser() = default;

        Parser& operator=(const Parser& other) = delete;
        Parser& operator=(Parser&& other) = delete;

    public:
        inline Token* GetToken(uint32_t n = 0) { return stream.GetTok(n); }
        inline Token* NextAndGetToken() { return stream.NextAndGetTok(); }
        inline bool NextToken(uint32_t n = 1) { return stream.Next(n); }
        Token* ExpectToken(TokenKind kind, uint32_t n = 0, const char* file = nullptr, const char* func = nullptr, uint32_t line = 0);
        
        bool Parse();

        Decl* ParseTopLevelDecl();
        Decl* ParseRecordLevelDecl();
        CompoundStmt* ParseCompoundStmt();

        TemplateDecl* ParseTemplateDecl();
        
        NamedDecl* ParseRecordDecl();
        FieldDecl* ParseFieldDecl();

        NamedDecl* ParseFunctionDecl();
        ParamDecl* ParseParamDecl();
        CompoundStmt* ParseFunctionBody(FunctionDecl* function);

        ReturnStmt* ParseReturnStmt();

        VarDecl* ParseVarDecl();

        std::optional<VarDecl::VarAttrBitfield> ParseVarAttrBitfield();

        WithFullLoc<QualType> ParseQualType();
        int TryParseQualType(int n = 0);
        
        WithFullLoc<Type*> ParseType();

        TemplateParameterList* ParseTemplateParameterList();
        TemplateArgumentList* ParseTemplateArgumentList();
        int TryParseTemplateArgumentList(int n = 0);

        Expr* ParseExpression();

        Expr* ParseBinaryExpression(Expr* lhs, int precedence);
        Expr* ParsePrefixExpression();
        Expr* ParsePostfixExpression();
        Expr* ParsePrimaryExpression();
        
        Expr* ParseParentExpression();
        Expr* ParseNumericConstantExpr();
        Expr* ParseIdentifierExpr();
        Expr* ParseCallExpr();
        Expr* ParseAggregateExpr();

    private:
        class TentativeParsingGuard {
        public:
            TentativeParsingGuard() = delete;
            TentativeParsingGuard(Parser* parser);
            TentativeParsingGuard(const TentativeParsingGuard& other) = delete;
            TentativeParsingGuard(TentativeParsingGuard&& other) = delete;
            ~TentativeParsingGuard();

            TentativeParsingGuard& operator=(const TentativeParsingGuard& other) = delete;
            TentativeParsingGuard& operator=(TentativeParsingGuard&& other) = delete;

            inline void Commit() {
                sp.Commit();
            }

        private:
            Parser* parser;
            TokenStreamSavePointGuard sp;
            bool isSupressing;
        };

        class ParserFlagGuard {
        public:
            ParserFlagGuard() = delete;
            ParserFlagGuard(Parser* parser, ParserFlag flags) : parser{ parser }, oldFlags{ parser->flags } {
                parser->flags = (ParserFlag)((uint32_t)parser->flags | (uint32_t)flags);
            }
            ParserFlagGuard(const ParserFlagGuard& other) = delete;
            ParserFlagGuard(ParserFlagGuard&& other) = delete;
            ~ParserFlagGuard() { parser->flags = oldFlags; }

            ParserFlagGuard& operator=(const ParserFlagGuard& other) = delete;
            ParserFlagGuard& operator=(ParserFlagGuard&& other) = delete;

        private:
            Parser* parser;
            ParserFlag oldFlags;
        };

        class ParserScopeGuard {
        public:
            ParserScopeGuard() = delete;
            ParserScopeGuard(Parser* parser, DeclContext* context);
            ParserScopeGuard(const ParserScopeGuard& other) = delete;
            ParserScopeGuard(ParserScopeGuard&& other) = delete;
            ~ParserScopeGuard();

            ParserScopeGuard& operator=(const ParserScopeGuard& other) = delete;
            ParserScopeGuard& operator=(ParserScopeGuard&& other) = delete;
            
        private:
            Parser* parser;
            DeclContext* context;
        };

    private:
        TokenStream& stream;
        Sema& sema;
        ParserFlag flags = ParserFlag::None;
    };

}