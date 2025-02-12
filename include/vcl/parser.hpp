#pragma once

#include <vcl/definition.hpp>
#include <vcl/lexer.hpp>
#include <vcl/ast.hpp>

#include <cstddef>
#include <cstdint>
#include <string>


namespace VCL {

    class Parser {
    public:
        ~Parser() = default;

        std::unique_ptr<ASTProgram> Parse(const std::string& source);


        static std::unique_ptr<Parser> Create();

    private:
        Parser() = default;

        std::unique_ptr<ASTStatement> ParseStatement(Lexer& lexer);
        std::unique_ptr<ASTStatement> ParseCompoundStatement(Lexer& lexer);
        std::unique_ptr<ASTStatement> ParseVariableAssignment(Lexer& lexer);
        std::unique_ptr<ASTStatement> ParseDeclarationStatement(Lexer& lexer);
        std::unique_ptr<ASTStatement> ParseVariableDeclarationStatement(Lexer& lexer, ASTTypeInfo type);
        std::unique_ptr<ASTStatement> ParseFunctionDeclarationStatement(Lexer& lexer, ASTTypeInfo type);
        std::unique_ptr<ASTFunctionPrototype> ParseFunctionPrototype(Lexer& lexer, ASTTypeInfo type);
        std::unique_ptr<ASTFunctionArgument> ParseFunctionArgument(Lexer& lexer);
        std::unique_ptr<ASTStatement> ParseReturnStatement(Lexer& lexer);

        std::unique_ptr<ASTExpression> ParseExpression(Lexer& lexer);
        std::unique_ptr<ASTExpression> ParsePrimaryExpression(Lexer& lexer);
        std::unique_ptr<ASTExpression> ParseParentExpression(Lexer& lexer);
        std::unique_ptr<ASTExpression> ParseLiteral(Lexer& lexer);
        std::unique_ptr<ASTExpression> ParseVariable(Lexer& lexer);
        std::unique_ptr<ASTExpression> ParseFunctionCall(Lexer& lexer);
        std::unique_ptr<ASTExpression> ParseBinaryOperationExpression(Lexer& lexer, int precedence, std::unique_ptr<ASTExpression> lhs);
        std::unique_ptr<ASTExpression> ParseUnaryOperationExpression(Lexer& lexer);
        
        ASTTypeInfo ParseTypeInfo(Lexer& lexer);
        void ParseTemplatedTypeInfo(Lexer& lexer, ASTTypeInfo& typeInfo);
        int GetTokenPrecedence(Token& token);
        void TokenUnaryToBinary(Token& token);
        void TokenBinaryToUnary(Token& token);
        uint8_t GetBinaryOperationIndexByTokenType(TokenType type);
        uint8_t GetUnaryOperationIndexByTokenType(TokenType type);
    };

}