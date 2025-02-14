#include <vcl/parser.hpp>

#include <vcl/definition.hpp>

#include <cstring>
#include <format>
#include <iostream>
#include <stdexcept>



std::unique_ptr<VCL::Parser> VCL::Parser::Create() {
    struct ParserCtor : public Parser {};
    return std::make_unique<ParserCtor>();
}

std::unique_ptr<VCL::ASTProgram> VCL::Parser::Parse(const std::string& source) {
    Lexer lexer{};
    if (!lexer.Tokenize(source))
        throw std::runtime_error{ "Lexer error! Unable to tokenize file because of undefined token." };

    std::vector<std::unique_ptr<ASTStatement>> program{};

    while (lexer.Peek().type != TokenType::ENDOFFILE) {
        std::unique_ptr<ASTStatement> statement = ParseStatement(lexer);
        program.emplace_back(std::move(statement));
    }

    return std::make_unique<ASTProgram>(std::move(program));
}

std::unique_ptr<VCL::ASTStatement> VCL::Parser::ParseStatement(Lexer& lexer) {
    Token currentToken = lexer.Peek();

    std::unique_ptr<VCL::ASTStatement> statement;

    switch (currentToken.type) {
        case TokenType::IN:
        case TokenType::OUT:
        case TokenType::CONST:
        case TokenType::FLOAT:
        case TokenType::INT:
        case TokenType::VFLOAT:
        case TokenType::BUFFER:
        case TokenType::ARRAY:
        case TokenType::VOID:
            statement = ParseDeclarationStatement(lexer);
            break;
        case TokenType::IDENTIFIER:
            if (lexer.Peek(1).type == TokenType::LPAR) {
                statement =  ParseFunctionCall(lexer);
                if (lexer.Peek().type != TokenType::SEMICOLON) {
                    throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\". Expecting semicolon after \"{}\".", 
                            lexer.Peek().line, lexer.Peek().position, lexer.Peek().name, lexer.Peek(-1).name) };
                }
                lexer.Consume(); //Consume semicolon
            }
            else if (lexer.Peek(1).type == TokenType::ASSIGNMENT)
                statement =  ParseVariableAssignment(lexer);
            else 
                throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\".", 
                    currentToken.line, currentToken.position, currentToken.name) };
            break;
        case TokenType::RETURN:
            statement = ParseReturnStatement(lexer);
            break;
        case TokenType::IF:
            statement = ParseIfStatement(lexer);
            break;
        case TokenType::WHILE:
            statement = ParseWhileStatement(lexer);
            break;
        case TokenType::FOR:
            statement = ParseForStatement(lexer);
            break;
        default:
            throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\".", 
                    currentToken.line, currentToken.position, currentToken.name) };
    }

    return std::move(statement);
}

std::unique_ptr<VCL::ASTStatement> VCL::Parser::ParseCompoundStatement(Lexer& lexer) {
    lexer.Consume(); //Eat left bracket

    std::vector<std::unique_ptr<ASTStatement>> statements{};

    while (true) {
        Token currentToken = lexer.Peek();
        if (currentToken.type == TokenType::RBRACKET)
            break;

        std::unique_ptr<VCL::ASTStatement> statement = ParseStatement(lexer);

        statements.emplace_back(std::move(statement));
    }

    lexer.Consume(); //Eat right bracket

    return std::make_unique<ASTCompoundStatement>(std::move(statements));
}

std::unique_ptr<VCL::ASTStatement> VCL::Parser::ParseVariableAssignment(Lexer& lexer) {
    Token identifierToken = lexer.Consume();
    switch (lexer.Peek().type) {
        case TokenType::ASSIGNMENT:
            lexer.Consume();
            break;
        default:
            throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\" after \"{}\".", 
                lexer.Peek().line, lexer.Peek().position, lexer.Peek().name, identifierToken.name) };
    }
    
    std::unique_ptr<VCL::ASTExpression> expression = ParseExpression(lexer);

    if (lexer.Peek().type != TokenType::SEMICOLON && lexer.Peek().type != TokenType::RPAR) {
        throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\". Expecting semicolon or right parenthesis after \"{}\".", 
            lexer.Peek().line, lexer.Peek().position, lexer.Peek().name, lexer.Peek(-1).name) };
    }
    lexer.Consume(); //Consume

    return std::make_unique<ASTVariableAssignment>(identifierToken.name, std::move(expression));
}

std::unique_ptr<VCL::ASTStatement> VCL::Parser::ParseDeclarationStatement(Lexer& lexer) {
    ASTTypeInfo type = ParseTypeInfo(lexer);
    
    Token identifierToken = lexer.Peek();
    if (identifierToken.type != TokenType::IDENTIFIER) {
        throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\". Missing declaration identifier.", 
            identifierToken.line, identifierToken.position, identifierToken.name) };
    }

    switch (lexer.Peek(1).type) {
        case TokenType::ASSIGNMENT:
        case TokenType::SEMICOLON:
            return ParseVariableDeclarationStatement(lexer, type);
        case TokenType::LPAR:
            return ParseFunctionDeclarationStatement(lexer, type);
        default:
            throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\" after \"{}\".", 
                identifierToken.line, identifierToken.position, lexer.Peek(1).name, identifierToken.name) };
    }
}

VCL::ASTTypeInfo VCL::Parser::ParseTypeInfo(Lexer& lexer) {
    ASTTypeInfo type{ ASTTypeInfo::QualifierFlag::NONE, ASTTypeInfo::TypeName::NONE, 0 };

    Token firstToken = lexer.Peek();
    bool c = true;
    while (c) {
        Token currentToken = lexer.Peek();

        switch (currentToken.type) {
            case TokenType::BUFFER:
            case TokenType::ARRAY:
                ParseTemplatedTypeInfo(lexer, type);
                c = false;
                break;
            case TokenType::IN:
                type.qualifiers |= ASTTypeInfo::QualifierFlag::IN;
                break;
            case TokenType::OUT:
                type.qualifiers |= ASTTypeInfo::QualifierFlag::OUT;
                break;
            case TokenType::CONST:
                type.qualifiers |= ASTTypeInfo::QualifierFlag::CONST;
                break;
            case TokenType::FLOAT:
                type.type = ASTTypeInfo::TypeName::FLOAT;
                c = false;
                break;
            case TokenType::INT:
                type.type = ASTTypeInfo::TypeName::INT;
                c = false;
                break;
            case TokenType::VFLOAT:
                type.type = ASTTypeInfo::TypeName::VFLOAT;
                c = false;
                break;
            case TokenType::VOID:
                type.type = ASTTypeInfo::TypeName::VOID;
                c = false;
                break;
            default:
                throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\". Expecting type or qualifier.", 
                    currentToken.line, currentToken.position, currentToken.name) };
        }

        lexer.Consume();
    }

    //Basic qualifier check
    
    //Can't be in and out
    if ((type.qualifiers & ASTTypeInfo::QualifierFlag::IN) && (type.qualifiers & ASTTypeInfo::QualifierFlag::OUT)) {
        throw std::runtime_error{ std::format("({}:{}): Type cannot have both qualifiers \"in\" and \"out\".", 
            firstToken.line, firstToken.position) };
    }
    //Can't be const and out
    if ((type.qualifiers & ASTTypeInfo::QualifierFlag::CONST) && (type.qualifiers & ASTTypeInfo::QualifierFlag::OUT)) {
        throw std::runtime_error{ std::format("({}:{}): Type that has the \"out\" qualifier cannot be const.", 
            firstToken.line, firstToken.position) };
    }

    return type;
}

void VCL::Parser::ParseTemplatedTypeInfo(Lexer& lexer, ASTTypeInfo& typeInfo) {
    Token currentToken = lexer.Consume();

    switch (currentToken.type) {
        case TokenType::BUFFER:
            typeInfo.qualifiers |= ASTTypeInfo::QualifierFlag::BUFFER;
            break;
        case TokenType::ARRAY:
            typeInfo.qualifiers |= ASTTypeInfo::QualifierFlag::ARRAY;
            break;
        default:
            throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\". Expecting buffer type.", 
                currentToken.line, currentToken.position, currentToken.name) };
    }

    currentToken = lexer.Consume();
    if (currentToken.type != TokenType::INFERIOR)
        throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\". Expecting template start.", 
            currentToken.line, currentToken.position, currentToken.name) };

    currentToken = lexer.Consume();
    switch (currentToken.type) {
        case TokenType::FLOAT:
            typeInfo.type = ASTTypeInfo::TypeName::FLOAT;
            break;
        case TokenType::VFLOAT:
            typeInfo.type = ASTTypeInfo::TypeName::VFLOAT;
            break;
        default:
            throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\".", 
                currentToken.line, currentToken.position, currentToken.name) };
    }

    currentToken = lexer.Peek();
    if (currentToken.type == TokenType::SUPERIOR) {
        typeInfo.arraySize = 0;
        return;
    } else if (currentToken.type != TokenType::COMA)
        throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\". Missing coma.", 
            currentToken.line, currentToken.position, currentToken.name) };
    lexer.Consume(); //Consume coma

    currentToken = lexer.Consume();
    if (currentToken.type != TokenType::LITERALINT)
        throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\". Missing buffer size.", 
            currentToken.line, currentToken.position, currentToken.name) };
    
    typeInfo.arraySize = std::stoi(std::string{ currentToken.name });

    currentToken = lexer.Peek();
    if (currentToken.type != TokenType::SUPERIOR)
        throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\". Expecting template end.", 
            currentToken.line, currentToken.position, currentToken.name) };
}

std::unique_ptr<VCL::ASTStatement> VCL::Parser::ParseVariableDeclarationStatement(Lexer& lexer, ASTTypeInfo type) {
    Token identifierToken = lexer.Peek();
    lexer.Consume();

    std::unique_ptr<ASTVariableDeclaration> statement;

    switch (lexer.Peek().type) {
        case TokenType::SEMICOLON:
            statement = std::make_unique<ASTVariableDeclaration>(type, identifierToken.name, nullptr);
            break;
        case TokenType::ASSIGNMENT:
            lexer.Consume(); //eat equal
            statement = std::make_unique<ASTVariableDeclaration>(type, identifierToken.name, ParseExpression(lexer));
            break;
        default:
            throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\".", 
                lexer.Peek().line, lexer.Peek().position, lexer.Peek().name) };
    }


    if (lexer.Peek().type != TokenType::SEMICOLON) {
        throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\". Expecting semicolon after \"{}\".", 
            lexer.Peek().line, lexer.Peek().position, lexer.Peek().name, lexer.Peek(-1).name) };
    }
    lexer.Consume(); //Consume semicolon

    return std::move(statement);
}

std::unique_ptr<VCL::ASTStatement> VCL::Parser::ParseFunctionDeclarationStatement(Lexer& lexer, ASTTypeInfo type) {
    std::unique_ptr<ASTFunctionPrototype> prototype{ ParseFunctionPrototype(lexer, type) };

    if (lexer.Peek().type == TokenType::SEMICOLON) {
        lexer.Consume(); //Consume semicolon
        return std::move(prototype);
    }

    if (lexer.Peek().type != TokenType::LBRACKET) {
        throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\". Expected semicolon or function definition.", 
            lexer.Peek().line, lexer.Peek().position, lexer.Peek().name) };
    }

    std::unique_ptr<ASTStatement> body = ParseCompoundStatement(lexer);

    return std::make_unique<ASTFunctionDeclaration>(std::move(prototype), std::move(body));
}

std::unique_ptr<VCL::ASTFunctionPrototype> VCL::Parser::ParseFunctionPrototype(Lexer& lexer, ASTTypeInfo type) {
    Token identifierToken = lexer.Peek();
    lexer.Consume();

    if (lexer.Peek().type != TokenType::LPAR) {
        throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\" after \"{}\". Expected function declaration.", 
            lexer.Peek().line, lexer.Peek().position, lexer.Peek().name, identifierToken.name) };
    }

    lexer.Consume(); //eat parenthesis

    std::vector<std::unique_ptr<ASTFunctionArgument>> arguments{};

    if (lexer.Peek().type == TokenType::RPAR) {
        lexer.Consume(); //eat parenthesis    
        return std::make_unique<ASTFunctionPrototype>(type, identifierToken.name, std::move(arguments));
    }

    while (true) {
        std::unique_ptr<ASTFunctionArgument> arg = ParseFunctionArgument(lexer);

        arguments.emplace_back(std::move(arg));

        Token currentToken = lexer.Peek();
        if (currentToken.type == TokenType::RPAR)
            break;
        if (currentToken.type != TokenType::COMA) {
            throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\". Expected coma or right parenthesis.", 
                currentToken.line, currentToken.position, currentToken.name) };
        }
        
        lexer.Consume();
    }

    lexer.Consume(); //eat parenthesis

    return std::make_unique<ASTFunctionPrototype>(type, identifierToken.name, std::move(arguments));
}

std::unique_ptr<VCL::ASTFunctionArgument> VCL::Parser::ParseFunctionArgument(Lexer& lexer) {
    ASTTypeInfo type = ParseTypeInfo(lexer);
    
    Token identifierToken = lexer.Peek();
    if (identifierToken.type != TokenType::IDENTIFIER) {
        throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\". Missing function argument identifier.", 
            identifierToken.line, identifierToken.position, identifierToken.name) };
    }

    lexer.Consume(); //Eat identifier token

    return std::make_unique<ASTFunctionArgument>(type, identifierToken.name);
}

std::unique_ptr<VCL::ASTStatement> VCL::Parser::ParseReturnStatement(Lexer& lexer) {
    lexer.Consume(); //eat return keword;
    std::unique_ptr<ASTReturnStatement> statement = std::make_unique<ASTReturnStatement>(ParseExpression(lexer));
    if (lexer.Peek().type != TokenType::SEMICOLON) {
        throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\". Expecting semicolon after \"{}\".", 
            lexer.Peek().line, lexer.Peek().position, lexer.Peek().name, lexer.Peek(-1).name) };
    }
    lexer.Consume(); //Consume semicolon
    return std::move(statement);
}

std::unique_ptr<VCL::ASTStatement> VCL::Parser::ParseIfStatement(Lexer& lexer) {
    lexer.Consume(); //Consume IF Token

    if (lexer.Peek().type != TokenType::LPAR)
        throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\".", 
            lexer.Peek().line, lexer.Peek().position, lexer.Peek().name) };
    lexer.Consume(); //Consume lpar

    std::unique_ptr<ASTExpression> condition = ParseExpression(lexer);

    
    if (lexer.Peek().type != TokenType::RPAR)
        throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\".", 
            lexer.Peek().line, lexer.Peek().position, lexer.Peek().name) };
    lexer.Consume(); //Consume rpar

    std::unique_ptr<ASTStatement> thenStmt = nullptr;

    if (lexer.Peek().type == TokenType::LBRACKET)
        thenStmt = ParseCompoundStatement(lexer);
    else
        thenStmt = ParseStatement(lexer);

    std::unique_ptr<ASTStatement> elseStmt = nullptr;

    if (lexer.Peek().type == TokenType::ELSE) {
        lexer.Consume();
        if (lexer.Peek().type == TokenType::LBRACKET)
            elseStmt = ParseCompoundStatement(lexer);
        else
            elseStmt = ParseStatement(lexer);
    }

    return std::make_unique<ASTIfStatement>(std::move(condition), std::move(thenStmt), std::move(elseStmt));
}

std::unique_ptr<VCL::ASTStatement> VCL::Parser::ParseWhileStatement(Lexer& lexer) {
    lexer.Consume(); //Consume While Token

    if (lexer.Peek().type != TokenType::LPAR)
        throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\".", 
            lexer.Peek().line, lexer.Peek().position, lexer.Peek().name) };
    lexer.Consume(); //Consume lpar

    std::unique_ptr<ASTExpression> condition = ParseExpression(lexer);
    
    if (lexer.Peek().type != TokenType::RPAR)
        throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\".", 
            lexer.Peek().line, lexer.Peek().position, lexer.Peek().name) };
    lexer.Consume(); //Consume rpar

    std::unique_ptr<ASTStatement> thenStmt = nullptr;

    if (lexer.Peek().type == TokenType::LBRACKET)
        thenStmt = ParseCompoundStatement(lexer);
    else
        thenStmt = ParseStatement(lexer);

    return std::make_unique<ASTWhileStatement>(std::move(condition), std::move(thenStmt));
}

std::unique_ptr<VCL::ASTStatement> VCL::Parser::ParseForStatement(Lexer& lexer) {
    lexer.Consume(); //Consume FOR Token

    if (lexer.Peek().type != TokenType::LPAR)
        throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\".", 
            lexer.Peek().line, lexer.Peek().position, lexer.Peek().name) };
    lexer.Consume(); //Consume lpar

    std::unique_ptr<ASTStatement> start = ParseStatement(lexer);
    std::unique_ptr<ASTExpression> condition = ParseExpression(lexer);

    if (lexer.Peek().type != TokenType::SEMICOLON)
        throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\".", 
            lexer.Peek().line, lexer.Peek().position, lexer.Peek().name) };
    lexer.Consume(); //Consume semicolon

    std::unique_ptr<ASTStatement> end = ParseStatement(lexer);
    std::unique_ptr<ASTStatement> thenStmt = nullptr;

    if (lexer.Peek().type == TokenType::LBRACKET)
        thenStmt = ParseCompoundStatement(lexer);
    else
        thenStmt = ParseStatement(lexer);

    return std::make_unique<ASTForStatement>(std::move(start), std::move(condition), std::move(end), std::move(thenStmt));
}

std::unique_ptr<VCL::ASTExpression> VCL::Parser::ParseExpression(Lexer& lexer) {
    std::unique_ptr<ASTExpression> expression = ParsePrimaryExpression(lexer);

    return ParseBinaryOperationExpression(lexer, 1000, std::move(expression));
}

std::unique_ptr<VCL::ASTExpression> VCL::Parser::ParsePrimaryExpression(Lexer& lexer) {
    Token currentToken = lexer.Peek();

    int currentTokenPrecedence = GetTokenPrecedence(currentToken);
    if (currentTokenPrecedence != -1)
        return ParseUnaryOperationExpression(lexer);

    switch (currentToken.type) {
        case TokenType::LITERALFLOAT:
        case TokenType::LITERALINT:
            return ParseLiteral(lexer);
        case TokenType::IDENTIFIER:
            if (lexer.Peek(1).type == TokenType::LPAR)
                return ParseFunctionCall(lexer);
            return ParseVariable(lexer);
        case TokenType::LPAR:
            return ParseParentExpression(lexer);
        default:
            throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\".", 
                currentToken.line, currentToken.position, currentToken.name) };
    }
}

std::unique_ptr<VCL::ASTExpression> VCL::Parser::ParseParentExpression(Lexer& lexer) {
    lexer.Consume(); //eat LPAR
    std::unique_ptr<ASTExpression> expression = ParseExpression(lexer);
    if (lexer.Peek().type != TokenType::RPAR) {
        throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\". Missing closing parenthesis.", 
            lexer.Peek().line, lexer.Peek().position, lexer.Peek().name) };
    }
    lexer.Consume(); //eat RPAR
    return std::move(expression);
}

std::unique_ptr<VCL::ASTExpression> VCL::Parser::ParseLiteral(Lexer& lexer) {
    Token literalToken = lexer.Consume();
    std::string literalStr{ literalToken.name };

    switch (literalToken.type) {
        case TokenType::LITERALFLOAT:
            return std::make_unique<ASTLiteralExpression>(std::stof(literalStr));
        case TokenType::LITERALINT:
            return std::make_unique<ASTLiteralExpression>(std::stoi(literalStr));
        default:
            throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\".", 
                literalToken.line, literalToken.position, literalToken.name) };
    }
}

std::unique_ptr<VCL::ASTExpression> VCL::Parser::ParseVariable(Lexer& lexer) {
    Token variableToken = lexer.Consume();
    return std::make_unique<ASTVariableExpression>(variableToken.name);
}

std::unique_ptr<VCL::ASTExpression> VCL::Parser::ParseFunctionCall(Lexer& lexer) {
    Token identifierToken = lexer.Consume();
    if (lexer.Peek().type != TokenType::LPAR) {
        throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\" after \"{}\". Expected function declaration.", 
            lexer.Peek().line, lexer.Peek().position, lexer.Peek().name, identifierToken.name) };
    }
    lexer.Consume();

    std::vector<std::unique_ptr<ASTExpression>> arguments{};

    if (lexer.Peek().type == TokenType::RPAR) {
        lexer.Consume(); //eat parenthesis
        return std::make_unique<ASTFunctionCall>(identifierToken.name, std::move(arguments));
    }

    while (true) {
        std::unique_ptr<ASTExpression> arg = ParseExpression(lexer);
        arguments.emplace_back(std::move(arg));

        Token currentToken = lexer.Peek();
        if (currentToken.type == TokenType::RPAR)
            break;
        if (currentToken.type != TokenType::COMA) {
            throw std::runtime_error{ std::format("({}:{}): Unexpected token \"{}\". Expected coma or right parenthesis.", 
                currentToken.line, currentToken.position, currentToken.name) };
        }
        
        lexer.Consume();
    }

    lexer.Consume(); //eat right parenthesis

    return std::make_unique<ASTFunctionCall>(identifierToken.name, std::move(arguments));
}

std::unique_ptr<VCL::ASTExpression> VCL::Parser::ParseBinaryOperationExpression(Lexer& lexer, int precedence, std::unique_ptr<ASTExpression> lhs) {
    while (true) {
        Token currentToken = lexer.Peek();
        TokenUnaryToBinary(currentToken);
        
        int currentTokenPrecedence = GetTokenPrecedence(currentToken);
        if (currentTokenPrecedence == -1)
            return lhs;
        if (currentTokenPrecedence > precedence)
            return lhs;

        lexer.Consume(); //eat operator

        std::unique_ptr<ASTExpression> rhs = ParsePrimaryExpression(lexer);

        Token nextToken = lexer.Peek();
        TokenUnaryToBinary(nextToken);

        int nextTokenPrecedence = GetTokenPrecedence(nextToken);
        if (currentTokenPrecedence > nextTokenPrecedence)
            rhs = ParseBinaryOperationExpression(lexer, currentTokenPrecedence - 1, std::move(rhs));
        
        uint8_t opIdx = GetBinaryOperationIndexByTokenType(currentToken.type);

        lhs = std::make_unique<ASTBinaryExpression>(opIdx, std::move(lhs), std::move(rhs));
    }
}

std::unique_ptr<VCL::ASTExpression> VCL::Parser::ParseUnaryOperationExpression(Lexer& lexer) {
    Token currentToken = lexer.Consume();
    uint8_t opIdx = GetUnaryOperationIndexByTokenType(currentToken.type);
    return std::make_unique<ASTUnaryExpression>(opIdx, ParsePrimaryExpression(lexer));
}

int VCL::Parser::GetTokenPrecedence(Token& token) {
#undef DEF
#define DEF(name, symbol, ...) { TokenType::name, __VA_ARGS__ },
    static struct TokenTypePrecedence {
        TokenType type;
        int precedence;
    } tokenTypePrecedence[] = {
        UNARY_OPERATOR_DEF
        BINARY_OPERATOR_DEF
        { TokenType::UNDEFINED, -1 }
    };

    for (uint32_t i = 0; i < sizeof(tokenTypePrecedence) / sizeof(TokenTypePrecedence); ++i) {
        if (tokenTypePrecedence[i].type == token.type) {
            return tokenTypePrecedence[i].precedence;
        }
    }

    return -1;
}

void VCL::Parser::TokenUnaryToBinary(Token& token) {
    switch (token.type) {
        case TokenType::PLUS:
            token.type = TokenType::ADDITION;
            break;
        case TokenType::MINUS:
            token.type = TokenType::SUBSTRACTION;
            break;
        default:
            break;
    }
}

void VCL::Parser::TokenBinaryToUnary(Token& token) {
    switch (token.type) {
        case TokenType::ADDITION:
            token.type = TokenType::PLUS;
            break;
        case TokenType::SUBSTRACTION:
            token.type = TokenType::MINUS;
            break;
        default:
            break;
    }
}

uint8_t VCL::Parser::GetBinaryOperationIndexByTokenType(TokenType type) {
#undef DEF
#define DEF(name, symbol, ...) TokenType::name,
    static TokenType binaryOps[] = {
        BINARY_OPERATOR_DEF
    };

    for (uint8_t i = 0; i < sizeof(binaryOps) / sizeof(TokenType); ++i)
        if (type == binaryOps[i])
            return (uint8_t)i;

    return 255;
}

uint8_t VCL::Parser::GetUnaryOperationIndexByTokenType(TokenType type) {
#undef DEF
#define DEF(name, symbol, ...) TokenType::name,
    static TokenType unaryOps[] = {
        UNARY_OPERATOR_DEF
    };

    for (uint8_t i = 0; i < sizeof(unaryOps) / sizeof(TokenType); ++i)
        if (type == unaryOps[i])
            return (uint8_t)i;

    return 255;
}