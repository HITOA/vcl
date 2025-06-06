#pragma once

#include <VCL/Definition.hpp>
#include <VCL/AST.hpp>
#include <VCL/Lexer.hpp>
#include <VCL/Logger.hpp>
#include <VCL/Source.hpp>

#include <cstddef>
#include <cstdint>
#include <string>


namespace VCL {

    /**
     * @brief This Parser class will take a source and produce an Abstract Syntax Tree used for compilation.
     */
    class Parser {
    public:
        Parser() = default;
        ~Parser() = default;

        /**
         * @brief Parse the given source and return the root node of a newly created Abstract Syntax Tree of the given source.
         */
        std::unique_ptr<ASTProgram> Parse(std::shared_ptr<Source> source);

        /**
         * @brief Set the logger class instance the parser will use to send error warning info and debug messages. 
         */
        void SetLogger(std::shared_ptr<Logger> logger);

        /**
         * @brief Create a Parser class instance.
         */
        static std::unique_ptr<Parser> Create(std::shared_ptr<Logger> logger = nullptr);

    private:
        std::unique_ptr<ASTStatement> ParseStatement(Lexer& lexer, bool ignoreTerminator = false);
        std::unique_ptr<ASTStatement> ParseCompoundStatement(Lexer& lexer);

        std::unique_ptr<ASTFunctionArgument> ParseFunctionArgument(Lexer& lexer);
        std::unique_ptr<ASTFunctionPrototype> ParseFunctionPrototype(Lexer& lexer, std::shared_ptr<TypeInfo> typeInfo);
        std::unique_ptr<ASTFunctionDeclaration> ParseFunctionDeclaration(Lexer& lexer, std::unique_ptr<ASTFunctionPrototype> prototype);
        std::unique_ptr<ASTStructureFieldDeclaration> ParseStructureFieldDeclaration(Lexer& lexer);
        std::unique_ptr<ASTTemplateParameterDeclaration> ParseTemplateParameterDeclaration(Lexer& lexer);
        std::unique_ptr<ASTStructureDeclaration> ParseStructureDeclaration(Lexer& lexer);
        std::unique_ptr<ASTTemplateFunctionDeclaration> ParseTemplateFunctionDeclaration(Lexer& lexer, std::shared_ptr<TypeInfo> typeInfo);
        std::unique_ptr<ASTReturnStatement> ParseReturnStatement(Lexer& lexer);
        std::unique_ptr<ASTIfStatement> ParseIfStatement(Lexer& lexer);
        std::unique_ptr<ASTForStatement> ParseForStatement(Lexer& lexer);
        std::unique_ptr<ASTWhileStatement> ParseWhileStatement(Lexer& lexer);
        std::unique_ptr<ASTBreakStatement> ParseBreakStatement(Lexer& lexer);

        std::unique_ptr<ASTExpression> ParseExpression(Lexer& lexer);
        std::unique_ptr<ASTExpression> ParseBinaryExpression(Lexer& lexer, std::unique_ptr<ASTExpression> lhs, int precedence);
        std::unique_ptr<ASTExpression> ParsePrefixExpression(Lexer& lexer);
        std::unique_ptr<ASTExpression> ParsePostfixExpression(Lexer& lexer);
        std::unique_ptr<ASTExpression> ParsePrimaryExpression(Lexer& lexer);
        
        std::unique_ptr<ASTExpression> ParseParentExpression(Lexer& lexer);
        std::unique_ptr<ASTLiteralExpression> ParseLiteralExpression(Lexer& lexer);
        std::unique_ptr<ASTVariableExpression> ParseVariableExpression(Lexer& lexer);
        std::unique_ptr<ASTVariableDeclaration> ParseVariableDeclaration(Lexer& lexer, std::shared_ptr<TypeInfo> typeInfo);
        std::unique_ptr<ASTFunctionCall> ParseFunctionCall(Lexer& lexer);
        std::unique_ptr<ASTFunctionCall> TryParseTemplatedFunctionCall(Lexer& lexer);

        std::shared_ptr<TypeInfo> TryParseTypeInfo(Lexer& lexer, TokenType expectedAheadTokenType);
        std::shared_ptr<TypeInfo> ParseTypeInfo(Lexer& lexer);
        std::shared_ptr<TemplateArgument> ParseTemplateArgument(Lexer& lexer);

    private:
        std::shared_ptr<Logger> logger = nullptr;
    };

}