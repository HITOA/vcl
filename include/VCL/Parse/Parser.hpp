#pragma once

#include <VCL/Core/CompilerContext.hpp>
#include <VCL/Lex/TokenStream.hpp>
#include <VCL/AST/ASTContext.hpp>
#include <VCL/AST/Decl.hpp>
#include <VCL/AST/Stmt.hpp>
#include <VCL/AST/Expr.hpp>
#include <VCL/AST/Template.hpp>
#include <VCL/Sema/Sema.hpp>

#include <optional>


namespace VCL {

    class Parser {
    public:
        enum class ParserFlag : uint32_t {
            None = 0x0,
            OnTemplateArgument = 0x1
        };

    public:
        Parser() = delete;
        Parser(TokenStream& stream, Sema& sema, CompilerContext& cc);
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
        Decl* ParseFunctionDeclOrVarDecl();
        CompoundStmt* ParseCompoundStmt();
        
        /** Parse a RecordDecl or a TemplateRecordDecl */
        NamedDecl* ParseAnyRecordDecl();
        FieldDecl* ParseFieldDecl();

        NamedDecl* ParseAnyFunctionDecl();
        FunctionDecl* ParseEndFunctionDecl(QualType type, IdentifierInfo* identifier, SourceRange range);
        NamedDecl* TryParseAnyFunctionDecl();
        ParamDecl* ParseParamDecl();
        CompoundStmt* ParseFunctionBody(FunctionDecl* function);

        ReturnStmt* ParseReturnStmt();

        VarDecl* ParseVarDecl();
        VarDecl* ParseEndVarDecl(VarDecl::VarAttrBitfield attr, QualType type, IdentifierInfo* identifier, SourceRange range);
        VarDecl* TryParseVarDecl();

        std::optional<VarDecl::VarAttrBitfield> ParseVarAttrBitfield();

        WithFullLoc<QualType> ParseQualType();
        WithFullLoc<QualType> TryParseQualType();
        
        WithFullLoc<Type*> ParseType();
        WithFullLoc<Type*> TryParseType();

        TemplateParameterList* ParseTemplateParameterList();
        TemplateArgumentList* ParseTemplateArgumentList();

        Expr* ParseExpression();
        Expr* TryParseExpression();

        Expr* ParseBinaryExpression(Expr* lhs, int precedence);
        Expr* ParsePrefixExpression();
        Expr* TryParsePrefixExpression();
        Expr* ParsePostfixExpression();
        Expr* ParsePrimaryExpression();
        
        //void ParseParentExpression();
        Expr* ParseNumericConstantExpr();
        Expr* ParseIdentifierExpr();

    private:
        class TentativeParsingGuard {
        public:
            TentativeParsingGuard() = delete;
            TentativeParsingGuard(Parser* parser) : parser{ parser }, sp{ parser->stream } {
                isSupressing = !parser->cc.GetDiagnosticReporter().GetSupressAll();
                if (isSupressing)
                    parser->cc.GetDiagnosticReporter().SetSupressAll(true);
            }
            TentativeParsingGuard(const TentativeParsingGuard& other) = delete;
            TentativeParsingGuard(TentativeParsingGuard&& other) = delete;
            ~TentativeParsingGuard() {
                if (isSupressing)
                    parser->cc.GetDiagnosticReporter().SetSupressAll(false);
            }

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
            ParserScopeGuard(Parser* parser, DeclContext* context) : parser{ parser }, context{ context } {
                parser->sema.PushDeclContextScope(context);
            }
            ParserScopeGuard(const ParserScopeGuard& other) = delete;
            ParserScopeGuard(ParserScopeGuard&& other) = delete;
            ~ParserScopeGuard() { parser->sema.PopDeclContextScope(context); }

            ParserScopeGuard& operator=(const ParserScopeGuard& other) = delete;
            ParserScopeGuard& operator=(ParserScopeGuard&& other) = delete;
            
        private:
            Parser* parser;
            DeclContext* context;
        };

    private:
        TokenStream& stream;
        Sema& sema;
        CompilerContext& cc;
        ParserFlag flags = ParserFlag::None;
    };

}