#pragma once

#include <VCL/Definition.hpp>
#include <VCL/AST.hpp>
#include <VCL/Lexer.hpp>

#include <cstddef>
#include <cstdint>
#include <string>


namespace VCL {

    /**
     * @brief This Parser class will take a source and produce an Abstract Syntax Tree used for compilation.
     */
    class Parser {
    public:
        ~Parser() = default;

        /**
         * @brief Parse the given source and produce an Abstract Syntax Tree returning its root node.
         */
        std::unique_ptr<ASTProgram> Parse(const std::string& source);

        /**
         * @brief Create a Parser class instance.
         */
        static std::unique_ptr<Parser> Create();

    private:
        std::unique_ptr<ASTStatement> ParseStatement(Lexer& lexer);
        std::unique_ptr<ASTStatement> ParseCompoundStatement(Lexer& lexer);
        std::unique_ptr<ASTExpression> ParseExpression(Lexer& lexer);
        std::unique_ptr<ASTExpression> ParsePrimaryExpression(Lexer& lexer);
        std::unique_ptr<ASTExpression> ParseBinaryOperationExpression(Lexer& lexer);

        std::unique_ptr<ASTVariableExpression> ParseVariableExpression(Lexer& lexer);
        std::unique_ptr<ASTVariableDeclaration> ParseVariableDeclaration(Lexer& lexer);
        std::unique_ptr<ASTVariableAssignment> ParseVariableAssignment(Lexer& lexer);

    private:
        Parser() = default;
    };

}