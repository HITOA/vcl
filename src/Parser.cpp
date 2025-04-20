#include <VCL/Parser.hpp>
#include <VCL/Error.hpp>

#include <expected>
#include <iostream>

template<typename T>
std::unique_ptr<T> FillASTStatementDebugInformation(std::unique_ptr<T>&& node, VCL::Token token) {
    node->location = token.location;
    return std::move(node);
}

int GetTokenPrecedence(VCL::Token token) {
#undef DEF
#define DEF(name, symbol, ...) { VCL::TokenType::name, __VA_ARGS__ },
    static struct TokenTypePrecedence {
        VCL::TokenType type;
        int precedence;
    } tokenTypePrecedence[] = {
        UNARY_OPERATOR_DEF
        BINARY_OPERATOR_DEF
    };

    for (size_t i = 0; i < sizeof(tokenTypePrecedence) / sizeof(TokenTypePrecedence); ++i) {
        if (tokenTypePrecedence[i].type == token.type)
            return tokenTypePrecedence[i].precedence;
    }

    return -1;
}

int GetUnaryTokenPrecedence(VCL::Token token) {
#undef DEF
#define DEF(name, symbol, ...) { VCL::TokenType::name, __VA_ARGS__ },
    static struct TokenTypePrecedence {
        VCL::TokenType type;
        int precedence;
    } tokenTypePrecedence[] = {
        UNARY_OPERATOR_DEF
    };

    for (size_t i = 0; i < sizeof(tokenTypePrecedence) / sizeof(TokenTypePrecedence); ++i) {
        if (tokenTypePrecedence[i].type == token.type)
            return tokenTypePrecedence[i].precedence;
    }
    
    return -1;
}

int GetBinaryTokenPrecedence(VCL::Token token) {
#undef DEF
#define DEF(name, symbol, ...) { VCL::TokenType::name, __VA_ARGS__ },
    static struct TokenTypePrecedence {
        VCL::TokenType type;
        int precedence;
    } tokenTypePrecedence[] = {
        BINARY_OPERATOR_DEF
    };

    for (size_t i = 0; i < sizeof(tokenTypePrecedence) / sizeof(TokenTypePrecedence); ++i) {
        if (tokenTypePrecedence[i].type == token.type)
            return tokenTypePrecedence[i].precedence;
    }

    return -1;
}

void TokenUnaryToBinary(VCL::Token& token) {
    switch (token.type) {
        case VCL::TokenType::PLUS:
            token.type = VCL::TokenType::ADDITION;
            break;
        case VCL::TokenType::MINUS:
            token.type = VCL::TokenType::SUBSTRACTION;
            break;
        default:
            break;
    }
}

void TokenBinaryToUnary(VCL::Token& token) {
    switch (token.type) {
        case VCL::TokenType::ADDITION:
            token.type = VCL::TokenType::PLUS;
            break;
        case VCL::TokenType::SUBSTRACTION:
            token.type = VCL::TokenType::MINUS;
            break;
        default:
            break;
    }
}

bool IsAssignmentOperatorToken(VCL::Token token) {
    return token.type == VCL::TokenType::ASSIGNMENT;
}

bool IsTypeInfoToken(VCL::Lexer& lexer) {
    VCL::Token currentToken = lexer.Peek();
    switch (currentToken.type)
    {
    //Qualifier(s)
    case VCL::TokenType::IN:
    case VCL::TokenType::OUT:
    case VCL::TokenType::CONST:
    //TypeName
    case VCL::TokenType::FLOAT:
    case VCL::TokenType::BOOLEAN:
    case VCL::TokenType::INT:
    case VCL::TokenType::VOID:
    case VCL::TokenType::VFLOAT:
    case VCL::TokenType::VBOOL:
    case VCL::TokenType::VINT:
        return true;
    case VCL::TokenType::IDENTIFIER:
        if (lexer.Peek(1).type == VCL::TokenType::IDENTIFIER)
            return true;
        return false;
    default:
        return false;
    }
}

bool IsLiteralToken(VCL::Token token) {
    switch (token.type) {
        case VCL::TokenType::LITERALFLOAT:
        case VCL::TokenType::LITERALINT:
            return true;
        default:
            return false;
    }
}

std::unique_ptr<VCL::ASTProgram> VCL::Parser::Parse(std::shared_ptr<Source> source) {
    Lexer lexer{};
    std::string path_str = source->path.string();

    if (!lexer.Tokenize(source))
        throw std::runtime_error{ std::format("Unable to tokenize file \"{}\"", path_str) };
    
    std::vector<std::unique_ptr<ASTStatement>> statements{};
    while (lexer.Peek().type != TokenType::ENDOFFILE) {
        std::unique_ptr<ASTStatement> statement = ParseStatement(lexer);
        if (!statement)
            throw std::runtime_error{ std::format("Error while parsing source file \"{}\"", path_str) };
        statements.emplace_back(std::move(statement));
    }

    return std::make_unique<ASTProgram>(std::move(statements), source);
}

void VCL::Parser::SetLogger(std::shared_ptr<Logger> logger)
{
    this->logger = logger;
}

std::unique_ptr<VCL::ASTStatement> VCL::Parser::ParseStatement(Lexer &lexer, bool ignoreTerminator)
{
    Token currentToken = lexer.Peek();
    
    /**
     * Statement can be:
     * - Variable Declaration (semicolon) [X]
     * - Function Prototype (semicolon) [X]
     * - Function Declaration (none) [X]
     * - Function Call (semicolon) [X]
     * - Variable Assignment (semicolon) [X]
     * - Return (semicolon) [X]
     * - If Block (none) [X]
     * - While Block (none) [X]
     * - For Block (none) [X]
     * - Break (semicolon) [X]
     * Statement either end with semicolon or none
     */

    TokenType expectedTerminatorTokenType = TokenType::SEMICOLON;
    std::unique_ptr<ASTStatement> statement = nullptr;

    if (IsTypeInfoToken(lexer)) { //Declaration
        TypeInfo typeInfo = ParseTypeInfo(lexer);
        if (lexer.Peek(1).type == TokenType::LPAR) { 
            std::unique_ptr<ASTFunctionPrototype> prototype = ParseFunctionPrototype(lexer, typeInfo);
            if (lexer.Peek().type == TokenType::LBRACKET) {
                statement = ParseFunctionDeclaration(lexer, std::move(prototype));
                expectedTerminatorTokenType = TokenType::UNDEFINED;
            }
            else statement = std::move(prototype);
        } else {
            statement = ParseVariableDeclaration(lexer, typeInfo);
        }
    } else if (currentToken.type == TokenType::IDENTIFIER) { //Assignment or Call
        if (lexer.Peek(1).type == TokenType::LPAR) statement = ParseFunctionCall(lexer);
        else statement = ParseVariableAssignment(lexer);
    } else if (currentToken.type == TokenType::STRUCT) {
        statement = ParseStructureDeclaration(lexer);
        expectedTerminatorTokenType = TokenType::UNDEFINED;
    } else if (currentToken.type == TokenType::RETURN) {
        statement = ParseReturnStatement(lexer);
    } else if (currentToken.type == TokenType::IF) {
        statement = ParseIfStatement(lexer);
        expectedTerminatorTokenType = TokenType::UNDEFINED;
    } else if (currentToken.type == TokenType::FOR) {
        statement = ParseForStatement(lexer);
        expectedTerminatorTokenType = TokenType::UNDEFINED;
    } else if (currentToken.type == TokenType::WHILE) {
        statement = ParseWhileStatement(lexer);
        expectedTerminatorTokenType = TokenType::UNDEFINED;
    } else if (currentToken.type == TokenType::BREAK) {
        statement = ParseBreakStatement(lexer);
    } else {
        throw Exception{ std::format("Unexpected token \'{}\'.", currentToken.name), currentToken.location };
    }

    if (statement == nullptr) {
        /*std::string error_message = FormatErrorMessage(currentToken.location, 
            std::format("Unexpected token \'{}\'.", currentToken.name), currentToken);
        if (logger)
            logger->Error("{}", error_message);*/
        return statement;
    }

    if (expectedTerminatorTokenType != TokenType::UNDEFINED && !ignoreTerminator) {
        Token terminator = lexer.Consume();
        if (terminator.type != expectedTerminatorTokenType)
            throw Exception{ std::format("Unexpected token \'{}\'. Expecting semicolon.", terminator.name), terminator.location };
    }

    return std::move(statement);
}

std::unique_ptr<VCL::ASTStatement> VCL::Parser::ParseCompoundStatement(Lexer& lexer) {
    lexer.Consume(); //LBRACKET
    std::vector<std::unique_ptr<ASTStatement>> statements{};
    while (lexer.Peek().type != TokenType::RBRACKET) {
        std::unique_ptr<ASTStatement> statement = ParseStatement(lexer);
        if (statement == nullptr)
            return nullptr;
        statements.emplace_back(std::move(statement));
    }
    lexer.Consume(); //RBRACKET
    return std::make_unique<ASTCompoundStatement>(std::move(statements));
}

std::unique_ptr<VCL::ASTExpression> VCL::Parser::ParseExpression(Lexer& lexer) {
    std::unique_ptr<ASTExpression> primary = ParsePrimaryExpression(lexer);
    if (!primary)
        return nullptr;
    return ParseBinaryOperationExpression(lexer, 1000, std::move(primary));
}

std::unique_ptr<VCL::ASTExpression> VCL::Parser::ParsePrimaryExpression(Lexer& lexer) {
    Token currentToken = lexer.Peek();

    /**
     * Expression can be:
     * - Unary Operation
     * - Variable Declaration [X]
     * - Variable Expression [X]
     * - Function Call [X]
     * - Variable Assignment [X]
     * - Literal Expression [X]
     */

    std::unique_ptr<ASTExpression> expression = nullptr;

    if (GetTokenPrecedence(currentToken) != -1) {
        expression = ParseUnaryOperationExpression(lexer);
    } else if (IsTypeInfoToken(lexer)) {
        TypeInfo typeInfo = ParseTypeInfo(lexer);
        expression = ParseVariableDeclaration(lexer, typeInfo);
    } else if (currentToken.type == TokenType::IDENTIFIER) {
        if (Token nextToken = lexer.Peek(1); IsAssignmentOperatorToken(nextToken)) {
            expression = ParseVariableAssignment(lexer);
        } else if (nextToken.type == TokenType::LPAR) {
            expression = ParseFunctionCall(lexer);
        } else {
            expression = ParseVariableExpression(lexer);
        }
    } else if (IsLiteralToken(currentToken)) {
        expression = ParseLiteralExpression(lexer);
    } else if (currentToken.type == TokenType::LPAR) {
        expression = ParseParentExpression(lexer);
    }

    if (expression == nullptr)
        return nullptr;
    
    return expression;
}

std::unique_ptr<VCL::ASTExpression> VCL::Parser::ParseParentExpression(Lexer& lexer) {
    lexer.Consume(); //LPAR
    std::unique_ptr<ASTExpression> expression = ParseExpression(lexer);
    if (expression == nullptr)
        return nullptr;
    Token closingParenthesisToken = lexer.Consume();
    if (closingParenthesisToken.type != TokenType::RPAR) 
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting closing parenthesis.", closingParenthesisToken.name), 
            closingParenthesisToken.location };
    return std::move(expression);
}

std::unique_ptr<VCL::ASTFunctionArgument> VCL::Parser::ParseFunctionArgument(Lexer& lexer) {
    Token currentToken = lexer.Peek();
    TypeInfo typeInfo = ParseTypeInfo(lexer);
    Token argumentIdentifierToken = lexer.Consume();
    if (argumentIdentifierToken.type != TokenType::IDENTIFIER)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting identifier.", argumentIdentifierToken.name), argumentIdentifierToken.location};
    return FillASTStatementDebugInformation(
        std::make_unique<ASTFunctionArgument>(typeInfo, argumentIdentifierToken.name), argumentIdentifierToken);
}

std::unique_ptr<VCL::ASTFunctionPrototype> VCL::Parser::ParseFunctionPrototype(Lexer& lexer, TypeInfo& typeInfo) {
    Token functionIdentifierToken = lexer.Consume();
    lexer.Consume(); //LPAR

    std::vector<std::unique_ptr<ASTFunctionArgument>> arguments{};

    if (lexer.Peek().type != TokenType::RPAR) {
        do {
            std::unique_ptr<ASTFunctionArgument> argument = ParseFunctionArgument(lexer);
            if (argument == nullptr)
                return nullptr;
            arguments.emplace_back(std::move(argument));
        } while(lexer.ConsumeIf(TokenType::COMA));
    }

    if (Token token = lexer.Consume(); token.type != TokenType::RPAR)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting closing parenthesis", token.name), token.location };

    return FillASTStatementDebugInformation(
        std::make_unique<ASTFunctionPrototype>(typeInfo, functionIdentifierToken.name, std::move(arguments)), functionIdentifierToken);
}

std::unique_ptr<VCL::ASTFunctionDeclaration> VCL::Parser::ParseFunctionDeclaration(Lexer& lexer, std::unique_ptr<ASTFunctionPrototype> prototype) {
    if (prototype == nullptr)
        return nullptr;
    std::unique_ptr<ASTStatement> body = ParseCompoundStatement(lexer);
    return std::make_unique<ASTFunctionDeclaration>(std::move(prototype), std::move(body));
}

std::unique_ptr<VCL::ASTStructureFieldDeclaration> VCL::Parser::ParseStructureFieldDeclaration(Lexer& lexer) {
    if (!IsTypeInfoToken(lexer))
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting type.", lexer.Peek().name), lexer.Peek().location};

    TypeInfo typeInfo = ParseTypeInfo(lexer);
    Token fieldIdentifierToken = lexer.Consume();

    if (fieldIdentifierToken.type != TokenType::IDENTIFIER)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting identifier.", fieldIdentifierToken.name), fieldIdentifierToken.location};

    if (Token token = lexer.Consume(); token.type != TokenType::SEMICOLON)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting semicolon.", token.name), token.location };

    return FillASTStatementDebugInformation(std::make_unique<ASTStructureFieldDeclaration>(typeInfo, fieldIdentifierToken.name), fieldIdentifierToken);
}

std::unique_ptr<VCL::ASTStructureDeclaration> VCL::Parser::ParseStructureDeclaration(Lexer& lexer) {
    Token structToken = lexer.Consume(); //Consume struct token
    Token structIdentifierToken = lexer.Consume();

    if (structIdentifierToken.type != TokenType::IDENTIFIER)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting identifier.", structIdentifierToken.name), structIdentifierToken.location};

    if (Token token = lexer.Consume(); token.type != TokenType::LBRACKET)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting bracket.", token.name), token.location };

    std::vector<std::unique_ptr<ASTStructureFieldDeclaration>> fields{};

    while (!lexer.ConsumeIf(TokenType::RBRACKET))
        fields.emplace_back(std::move(ParseStructureFieldDeclaration(lexer)));

    return FillASTStatementDebugInformation(std::make_unique<ASTStructureDeclaration>(structIdentifierToken.name, std::move(fields)), structToken);
}

std::unique_ptr<VCL::ASTReturnStatement> VCL::Parser::ParseReturnStatement(Lexer& lexer) {
    Token returnToken = lexer.Consume(); //RETURN
    std::unique_ptr<ASTExpression> expression = ParseExpression(lexer);
    return FillASTStatementDebugInformation(std::make_unique<ASTReturnStatement>(std::move(expression)), returnToken);
}

std::unique_ptr<VCL::ASTIfStatement> VCL::Parser::ParseIfStatement(Lexer& lexer) {
    Token ifToken = lexer.Consume(); //Consume if token

    if (Token token = lexer.Consume(); token.type != TokenType::LPAR)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting opening parenthesis", token.name), token.location };

    std::unique_ptr<ASTExpression> condition = ParseExpression(lexer);

    if (Token token = lexer.Consume(); token.type != TokenType::RPAR)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting closing parenthesis", token.name), token.location };

    std::unique_ptr<ASTStatement> thenStatement = nullptr;

    if (lexer.Peek().type == TokenType::LBRACKET)
        thenStatement = ParseCompoundStatement(lexer);
    else
        thenStatement = ParseStatement(lexer);

    std::unique_ptr<ASTStatement> elseStatement = nullptr;

    if (lexer.Peek().type == TokenType::ELSE) {
        lexer.Consume();
        if (lexer.Peek().type == TokenType::LBRACKET)
            elseStatement = ParseCompoundStatement(lexer);
        else
            elseStatement = ParseStatement(lexer);
    }

    return FillASTStatementDebugInformation(std::make_unique<ASTIfStatement>(std::move(condition), 
        std::move(thenStatement), std::move(elseStatement)), ifToken);
}

std::unique_ptr<VCL::ASTForStatement> VCL::Parser::ParseForStatement(Lexer& lexer) {
    Token forToken = lexer.Consume(); //Consume for token

    if (Token token = lexer.Consume(); token.type != TokenType::LPAR)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting opening parenthesis", token.name), token.location };

    std::unique_ptr<ASTStatement> start = ParseStatement(lexer);
    std::unique_ptr<ASTExpression> condition = ParseExpression(lexer);

    if (Token token = lexer.Consume(); token.type != TokenType::SEMICOLON)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting semicolon", token.name), token.location };

    std::unique_ptr<ASTStatement> end = ParseStatement(lexer, true);

    if (Token token = lexer.Consume(); token.type != TokenType::RPAR)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting closing parenthesis", token.name), token.location };

    std::unique_ptr<ASTStatement> thenStatement = nullptr;

    if (lexer.Peek().type == TokenType::LBRACKET)
        thenStatement = ParseCompoundStatement(lexer);
    else
        thenStatement = ParseStatement(lexer);

    return FillASTStatementDebugInformation(std::make_unique<ASTForStatement>(std::move(start), 
        std::move(condition), std::move(end), std::move(thenStatement)), forToken);
}

std::unique_ptr<VCL::ASTWhileStatement> VCL::Parser::ParseWhileStatement(Lexer& lexer) {
    Token whileToken = lexer.Consume(); //Consume while token

    if (Token token = lexer.Consume(); token.type != TokenType::LPAR)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting opening parenthesis", token.name), token.location };

    std::unique_ptr<ASTExpression> condition = ParseExpression(lexer);

    if (Token token = lexer.Consume(); token.type != TokenType::RPAR)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting closing parenthesis", token.name), token.location };
    
    std::unique_ptr<ASTStatement> thenStatement = nullptr;

    if (lexer.Peek().type == TokenType::LBRACKET)
        thenStatement = ParseCompoundStatement(lexer);
    else
        thenStatement = ParseStatement(lexer);

    return FillASTStatementDebugInformation(std::make_unique<ASTWhileStatement>(std::move(condition), 
        std::move(thenStatement)), whileToken);
}

std::unique_ptr<VCL::ASTBreakStatement> VCL::Parser::ParseBreakStatement(Lexer& lexer) {
    Token breakToken = lexer.Consume(); //Consume break token
    return FillASTStatementDebugInformation(std::make_unique<ASTBreakStatement>(), breakToken);
}

std::unique_ptr<VCL::ASTExpression> VCL::Parser::ParseBinaryOperationExpression(Lexer& lexer, int precedence, std::unique_ptr<ASTExpression> lhs) {
    while (true) {
        Token currentOperatorToken = lexer.Peek();
        TokenUnaryToBinary(currentOperatorToken);

        int currentOperatorTokenPrecedence = GetBinaryTokenPrecedence(currentOperatorToken);
        if (currentOperatorTokenPrecedence == -1 || currentOperatorTokenPrecedence > precedence)
            return lhs;

        lexer.Consume(); //Operator

        std::unique_ptr<ASTExpression> rhs = ParsePrimaryExpression(lexer);
        if (!rhs)
            return nullptr;

        Token nextOperatorToken = lexer.Peek();
        TokenUnaryToBinary(nextOperatorToken);

        int nextOperatorTokenPrecedence = GetTokenPrecedence(nextOperatorToken);
        if (currentOperatorTokenPrecedence > nextOperatorTokenPrecedence)
            rhs = ParseBinaryOperationExpression(lexer, currentOperatorTokenPrecedence - 1, std::move(rhs));
        
        if (rhs == nullptr)
            return nullptr;

        BinaryOpType opType = (BinaryOpType)((int)currentOperatorToken.type - (int)TokenType::MULTIPLICATION);

        lhs = std::make_unique<ASTBinaryExpression>(opType, std::move(lhs), std::move(rhs));
    }
}

std::unique_ptr<VCL::ASTUnaryExpression> VCL::Parser::ParseUnaryOperationExpression(Lexer& lexer) {
    Token currentOperatorToken = lexer.Consume();
    TokenBinaryToUnary(currentOperatorToken);
    if (GetUnaryTokenPrecedence(currentOperatorToken) == -1)
        throw Exception{ std::format("Invalid unary operator \'{}\'.", currentOperatorToken.name), currentOperatorToken.location };
    UnaryOpType opType = (UnaryOpType)((int)currentOperatorToken.type - (int)TokenType::PLUS);
    std::unique_ptr<ASTExpression> expression = ParsePrimaryExpression(lexer);
    if (expression == nullptr)
        return nullptr;
    return std::make_unique<ASTUnaryExpression>(opType, std::move(expression));
}

std::unique_ptr<VCL::ASTLiteralExpression> VCL::Parser::ParseLiteralExpression(Lexer& lexer) {
    Token literalToken = lexer.Consume();
    std::string literalStr{ literalToken.name };
    switch (literalToken.type)
    {
    case TokenType::LITERALFLOAT:
        return std::make_unique<ASTLiteralExpression>(std::stof(literalStr));
    case TokenType::LITERALINT:
        return std::make_unique<ASTLiteralExpression>(std::stoi(literalStr));
    default:
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting literal expression.", literalToken.name), literalToken.location };
    }
}

std::unique_ptr<VCL::ASTVariableExpression> VCL::Parser::ParseVariableExpression(Lexer& lexer) {
    Token variableIdentifierToken = lexer.Consume();
    return FillASTStatementDebugInformation(std::make_unique<ASTVariableExpression>(variableIdentifierToken.name), variableIdentifierToken);
}

std::unique_ptr<VCL::ASTVariableDeclaration> VCL::Parser::ParseVariableDeclaration(Lexer& lexer, TypeInfo& typeInfo) {
    Token variableIdentifierToken = lexer.Consume();
    std::unique_ptr<ASTExpression> expression = nullptr;
    if (lexer.Peek().type == TokenType::ASSIGNMENT) {
        lexer.Consume();
        expression = ParseExpression(lexer);
    }
    return FillASTStatementDebugInformation(
            std::make_unique<ASTVariableDeclaration>(typeInfo, variableIdentifierToken.name, std::move(expression)), variableIdentifierToken);
}

std::unique_ptr<VCL::ASTVariableAssignment> VCL::Parser::ParseVariableAssignment(Lexer& lexer) {
    Token variableIdentifierToken = lexer.Consume();
    Token operatorToken = lexer.Consume();
    std::unique_ptr<ASTExpression> expression = ParseExpression(lexer);
    if (!expression)
        return nullptr;
    return FillASTStatementDebugInformation(
        std::make_unique<ASTVariableAssignment>(variableIdentifierToken.name, std::move(expression)), 
        variableIdentifierToken);
}

std::unique_ptr<VCL::ASTFunctionCall> VCL::Parser::ParseFunctionCall(Lexer& lexer) {
    Token functionIdentifierToken = lexer.Consume();
    lexer.Consume(); //LPAR

    std::vector<std::unique_ptr<ASTExpression>> arguments{};

    if (lexer.Peek().type != TokenType::RPAR) {
        do {
            std::unique_ptr<ASTExpression> expression = ParseExpression(lexer);
            if (expression == nullptr)
                return nullptr;
            arguments.emplace_back(std::move(expression));
        } while (lexer.ConsumeIf(TokenType::COMA));
    }

    if (Token token = lexer.Consume(); token.type != TokenType::RPAR)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting closing parenthesis", token.name), token.location };

    return FillASTStatementDebugInformation(std::make_unique<ASTFunctionCall>(functionIdentifierToken.name, std::move(arguments)), functionIdentifierToken);
}

VCL::TypeInfo VCL::Parser::ParseTypeInfo(Lexer& lexer) {
    TypeInfo typeInfo{};
    bool complete = false;
    Token lastTypeInfoToken;
    while (!complete) {
        Token currentTypeInfoToken = lexer.Consume();
        switch (currentTypeInfoToken.type) {
            //Qualifier(s)
            case TokenType::IN:
                typeInfo.qualifiers |= TypeInfo::QualifierFlag::IN;
                break;
            case TokenType::OUT:
                typeInfo.qualifiers |= TypeInfo::QualifierFlag::OUT;
                break;
            case TokenType::CONST:
                typeInfo.qualifiers |= TypeInfo::QualifierFlag::CONST;
                break;
            //TypeName
            case TokenType::FLOAT:
                typeInfo.type = TypeInfo::TypeName::FLOAT;
                complete = true;
                break;
            case TokenType::BOOLEAN:
                typeInfo.type = TypeInfo::TypeName::BOOLEAN;
                complete = true;
                break;
            case TokenType::INT:
                typeInfo.type = TypeInfo::TypeName::INT;
                complete = true;
                break;
            case TokenType::VOID:
                typeInfo.type = TypeInfo::TypeName::VOID;
                complete = true;
                break;
            case TokenType::VFLOAT:
                typeInfo.type = TypeInfo::TypeName::VFLOAT;
                complete = true;
                break;
            case TokenType::VBOOL:
                typeInfo.type = TypeInfo::TypeName::VBOOL;
                complete = true;
                break;
            case TokenType::VINT:
                typeInfo.type = TypeInfo::TypeName::VINT;
                complete = true;
                break;
            case TokenType::IDENTIFIER:
                typeInfo.type = TypeInfo::TypeName::CUSTOM;
                typeInfo.name = currentTypeInfoToken.name;
                complete = true;
                break;
            default:
                if (lastTypeInfoToken.type != TokenType::UNDEFINED)
                    throw Exception{ std::format("Unexpected token \'{}\' after \'{}\'. Incomplete type error.",
                        currentTypeInfoToken.name, lastTypeInfoToken.name), currentTypeInfoToken.location };
                else
                    throw Exception{ std::format("Unexpected token \'{}\'. Incomplete type error.",
                        currentTypeInfoToken.name), currentTypeInfoToken.location };
        }
        lastTypeInfoToken = currentTypeInfoToken;
    }
    return typeInfo;
}

std::unique_ptr<VCL::Parser> VCL::Parser::Create(std::shared_ptr<Logger> logger) {
    std::unique_ptr<VCL::Parser> parser = std::make_unique<Parser>();
    parser->SetLogger(logger);
    return std::move(parser);
}