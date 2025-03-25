#include <VCL/Parser.hpp>



template<typename T>
std::unique_ptr<T> FillASTStatementDebugInformation(std::unique_ptr<T> node, VCL::Token& token) {
    node->source = token.source;
    node->position = token.position;
    node->line = token.line;
    return std::move(node);
}

std::unique_ptr<VCL::ASTProgram> VCL::Parser::Parse(const std::string& source) {
    Lexer lexer{};
    if (!lexer.Tokenize(source))
        throw std::runtime_error{ "Unable to tokenize source!" };
    
    std::vector<std::unique_ptr<ASTStatement>> statements{};
    while (lexer.Peek().type != TokenType::ENDOFFILE) {
        std::unique_ptr<ASTStatement> statement = ParseStatement(lexer);
        statements.emplace_back(std::move(statement));
    }

    return std::make_unique<ASTProgram>(std::move(statements));
}

std::unique_ptr<VCL::ASTStatement> VCL::Parser::ParseStatement(Lexer& lexer) {
    Token currentToken = lexer.Peek();
    
    /**
     * Statement can be:
     * - Variable Declaration (semicolon)
     * - Function Prototype (semicolon)
     * - Function Declaration (bracket)
     * - Function Call (semicolon)
     * - Variable Assignment (semicolon)
     * - Return (semicolon)
     * - If Block (bracket)
     * - While Block (bracket)
     * - For Block (bracket)
     * Statement either end with closing bracket or semicolon
     */

}

std::unique_ptr<VCL::ASTStatement> VCL::Parser::ParseCompoundStatement(Lexer& lexer) {

}

std::unique_ptr<VCL::ASTExpression> VCL::Parser::ParseExpression(Lexer& lexer) {
    std::unique_ptr<ASTExpression> primary = ParsePrimaryExpression(lexer);
    if (lexer.Peek().type == TokenType::SEMICOLON)
        return primary;
    return ParseBinaryOperationExpression(lexer);
}

std::unique_ptr<VCL::ASTExpression> VCL::Parser::ParsePrimaryExpression(Lexer& lexer) {
    Token currentToken = lexer.Peek();

    /**
     * Expression can be:
     * - Variable Expression
     * - Function Call
     * - Variable Declaration
     * - Variable Assignment
     * - Literal Expression
     */
}

std::unique_ptr<VCL::ASTExpression> VCL::Parser::ParseBinaryOperationExpression(Lexer& lexer) {

}

std::unique_ptr<VCL::ASTVariableExpression> VCL::Parser::ParseVariableExpression(Lexer& lexer) {
    if (lexer.Peek().type != TokenType::IDENTIFIER)
        return nullptr;
    Token variableIdentifierToken = lexer.Consume();
    return FillASTStatementDebugInformation(std::make_unique<ASTVariableExpression>(variableIdentifierToken.name), variableIdentifierToken);
}

std::unique_ptr<VCL::ASTVariableDeclaration> VCL::Parser::ParseVariableDeclaration(Lexer& lexer) {

}

std::unique_ptr<VCL::ASTVariableAssignment> VCL::Parser::ParseVariableAssignment(Lexer& lexer) {
    if (lexer.Peek().type != TokenType::IDENTIFIER)
        return nullptr;
}

std::unique_ptr<VCL::Parser> VCL::Parser::Create() {
    return std::make_unique<Parser>();
}