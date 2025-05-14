#include <VCL/Parser.hpp>
#include <VCL/Error.hpp>

#include <expected>
#include <iostream>

template<typename T>
std::unique_ptr<T> FillASTStatementDebugInformation(std::unique_ptr<T>&& node, VCL::Token token) {
    node->location = token.location;
    return std::move(node);
}

bool IsAssignmentOperatorToken(VCL::Token token) {
    return token.type == VCL::TokenType::Assignment;
}

bool IsTypeInfoToken(VCL::Lexer& lexer) {
    VCL::Token currentToken = lexer.Peek();
    switch (currentToken.type)
    {
    //Qualifier(s)
    case VCL::TokenType::In:
    case VCL::TokenType::Out:
    case VCL::TokenType::Const:
    //TypeName
    case VCL::TokenType::Float:
    case VCL::TokenType::Bool:
    case VCL::TokenType::Int:
    case VCL::TokenType::Void:
    case VCL::TokenType::VectorFloat:
    case VCL::TokenType::VectorBool:
    case VCL::TokenType::VectorInt:
    case VCL::TokenType::Array:
    case VCL::TokenType::Span:
        return true;
    case VCL::TokenType::Identifier:
        if (lexer.Peek(1).type == VCL::TokenType::Identifier)
            return true;
        return false;
    default:
        return false;
    }
}

bool IsLiteralToken(VCL::Token token) {
    switch (token.type) {
        case VCL::TokenType::LiteralFloat:
        case VCL::TokenType::LiteralInt:
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
    while (lexer.Peek().type != TokenType::EndOfFile) {
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
     * - Structure Declaration (none) [X]
     * - Return (semicolon) [X]
     * - If Block (none) [X]
     * - While Block (none) [X]
     * - For Block (none) [X]
     * - Break (semicolon) [X]
     * Statement either end with semicolon or none
     */

    TokenType expectedTerminatorTokenType = TokenType::Semicolon;
    std::unique_ptr<ASTStatement> statement = nullptr;

    if (IsTypeInfoToken(lexer)) { //Declaration
        std::shared_ptr<TypeInfo> typeInfo = ParseTypeInfo(lexer);
        if (lexer.Peek(1).type == TokenType::Less) {
            statement = ParseTemplateFunctionDeclaration(lexer, typeInfo);
            expectedTerminatorTokenType = TokenType::Undefined;
        } else if (lexer.Peek(1).type == TokenType::LPar) { 
            std::unique_ptr<ASTFunctionPrototype> prototype = ParseFunctionPrototype(lexer, typeInfo);
            if (lexer.Peek().type == TokenType::LBracket) {
                statement = ParseFunctionDeclaration(lexer, std::move(prototype));
                expectedTerminatorTokenType = TokenType::Undefined;
            }
            else statement = std::move(prototype);
        } else {
            statement = ParseVariableDeclaration(lexer, typeInfo);
        }
    } else if (currentToken.type == TokenType::Struct) {
        statement = ParseStructureDeclaration(lexer);
        expectedTerminatorTokenType = TokenType::Undefined;
    } else if (currentToken.type == TokenType::Return) {
        statement = ParseReturnStatement(lexer);
    } else if (currentToken.type == TokenType::If) {
        statement = ParseIfStatement(lexer);
        expectedTerminatorTokenType = TokenType::Undefined;
    } else if (currentToken.type == TokenType::For) {
        statement = ParseForStatement(lexer);
        expectedTerminatorTokenType = TokenType::Undefined;
    } else if (currentToken.type == TokenType::While) {
        statement = ParseWhileStatement(lexer);
        expectedTerminatorTokenType = TokenType::Undefined;
    } else if (currentToken.type == TokenType::Break) {
        statement = ParseBreakStatement(lexer);
    } else {
        statement = ParseExpression(lexer);
    }

    if (statement == nullptr) {
        /*std::string error_message = FormatErrorMessage(currentToken.location, 
            std::format("Unexpected token \'{}\'.", currentToken.name), currentToken);
        if (logger)
            logger->Error("{}", error_message);*/
        return statement;
    }

    if (expectedTerminatorTokenType != TokenType::Undefined && !ignoreTerminator) {
        Token terminator = lexer.Consume();
        if (terminator.type != expectedTerminatorTokenType)
            throw Exception{ std::format("Unexpected token \'{}\'. Expecting semicolon.", terminator.name), terminator.location };
    }

    return std::move(statement);
}

std::unique_ptr<VCL::ASTStatement> VCL::Parser::ParseCompoundStatement(Lexer& lexer) {
    lexer.Consume(); //LBRACKET
    std::vector<std::unique_ptr<ASTStatement>> statements{};
    while (lexer.Peek().type != TokenType::RBracket) {
        std::unique_ptr<ASTStatement> statement = ParseStatement(lexer);
        if (statement == nullptr)
            return nullptr;
        statements.emplace_back(std::move(statement));
    }
    lexer.Consume(); //RBRACKET
    return std::make_unique<ASTCompoundStatement>(std::move(statements));
}

std::unique_ptr<VCL::ASTFunctionArgument> VCL::Parser::ParseFunctionArgument(Lexer& lexer) {
    Token currentToken = lexer.Peek();
    std::shared_ptr<TypeInfo> typeInfo = ParseTypeInfo(lexer);
    Token argumentIdentifierToken = lexer.Consume();
    if (argumentIdentifierToken.type != TokenType::Identifier)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting identifier.", argumentIdentifierToken.name), argumentIdentifierToken.location};
    return FillASTStatementDebugInformation(
        std::make_unique<ASTFunctionArgument>(typeInfo, argumentIdentifierToken.name), argumentIdentifierToken);
}

std::unique_ptr<VCL::ASTFunctionPrototype> VCL::Parser::ParseFunctionPrototype(Lexer& lexer, std::shared_ptr<TypeInfo> typeInfo) {
    Token functionIdentifierToken = lexer.Consume();
    lexer.Consume(); //LPAR

    std::vector<std::unique_ptr<ASTFunctionArgument>> arguments{};

    if (lexer.Peek().type != TokenType::RPar) {
        do {
            std::unique_ptr<ASTFunctionArgument> argument = ParseFunctionArgument(lexer);
            if (argument == nullptr)
                return nullptr;
            arguments.emplace_back(std::move(argument));
        } while(lexer.ConsumeIf(TokenType::Coma));
    }

    if (Token token = lexer.Consume(); token.type != TokenType::RPar)
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
    if (!IsTypeInfoToken(lexer) && lexer.Peek().type != TokenType::Identifier)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting type.", lexer.Peek().name), lexer.Peek().location};

    std::shared_ptr<TypeInfo> typeInfo = ParseTypeInfo(lexer);
    Token fieldIdentifierToken = lexer.Consume();

    if (fieldIdentifierToken.type != TokenType::Identifier)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting identifier.", fieldIdentifierToken.name), fieldIdentifierToken.location};

    if (Token token = lexer.Consume(); token.type != TokenType::Semicolon)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting semicolon.", token.name), token.location };

    return FillASTStatementDebugInformation(std::make_unique<ASTStructureFieldDeclaration>(typeInfo, fieldIdentifierToken.name), fieldIdentifierToken);
}

std::unique_ptr<VCL::ASTTemplateParameterDeclaration> VCL::Parser::ParseTemplateParameterDeclaration(Lexer& lexer) {
    Token typeToken = lexer.Consume();

    TemplateArgument::TemplateValueType type;

    switch (typeToken.type)
    {
    case TokenType::Typename:
        type = TemplateArgument::TemplateValueType::Typename;
        break;
    case TokenType::Int:
        type = TemplateArgument::TemplateValueType::Int;
        break;
    default:
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting parameter type.", typeToken.name), typeToken.location};
    }

    Token identifierToken = lexer.Consume();
    if (identifierToken.type != TokenType::Identifier)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting parameter identifier.", identifierToken.name), identifierToken.location};

    return FillASTStatementDebugInformation(std::make_unique<ASTTemplateParameterDeclaration>(identifierToken.name, type), typeToken);
}

std::unique_ptr<VCL::ASTStructureDeclaration> VCL::Parser::ParseStructureDeclaration(Lexer& lexer) {
    Token structToken = lexer.Consume(); //Consume struct token
    Token structIdentifierToken = lexer.Consume();

    if (structIdentifierToken.type != TokenType::Identifier)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting identifier.", structIdentifierToken.name), structIdentifierToken.location};

    bool generic = false;
    std::vector<std::unique_ptr<ASTTemplateParameterDeclaration>> parameters{};

    if (lexer.ConsumeIf(TokenType::Less)) {
        generic = true;
        do {
            parameters.push_back(std::move(ParseTemplateParameterDeclaration(lexer)));
        } while (lexer.ConsumeIf(TokenType::Coma));
        
        if (Token token = lexer.Consume(); token.type != TokenType::Greater)
            throw Exception{ std::format("Unexpected token \'{}\'.", token.name), token.location };
    }

    if (Token token = lexer.Consume(); token.type != TokenType::LBracket)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting bracket.", token.name), token.location };

    std::vector<std::unique_ptr<ASTStructureFieldDeclaration>> fields{};

    while (!lexer.ConsumeIf(TokenType::RBracket))
        fields.emplace_back(std::move(ParseStructureFieldDeclaration(lexer)));

    if (generic)
        return FillASTStatementDebugInformation(std::make_unique<ASTTemplateDeclaration>(
            structIdentifierToken.name, std::move(parameters), std::move(fields)), structToken);
    else
        return FillASTStatementDebugInformation(std::make_unique<ASTStructureDeclaration>(structIdentifierToken.name, std::move(fields)), structToken);
}

std::unique_ptr<VCL::ASTTemplateFunctionDeclaration> VCL::Parser::ParseTemplateFunctionDeclaration(Lexer& lexer, std::shared_ptr<TypeInfo> typeInfo) {
    Token functionIdentifierToken = lexer.Consume();
    lexer.Consume(); //Less

    std::vector<std::unique_ptr<ASTTemplateParameterDeclaration>> parameters{};

    do {
        parameters.push_back(std::move(ParseTemplateParameterDeclaration(lexer)));
    } while(lexer.ConsumeIf(TokenType::Coma));

    if (Token token = lexer.Consume(); token.type != TokenType::Greater)
        throw Exception{ std::format("Unexpected token \'{}\'.", token.name), token.location };

    std::vector<std::unique_ptr<ASTFunctionArgument>> arguments{};

    if (Token token = lexer.Consume(); token.type != TokenType::LPar)
        throw Exception{ std::format("Unexpected token \'{}\'.", token.name), token.location };

    if (lexer.Peek().type != TokenType::RPar) {
        do {
            std::unique_ptr<ASTFunctionArgument> argument = ParseFunctionArgument(lexer);
            if (argument == nullptr)
                return nullptr;
            arguments.emplace_back(std::move(argument));
        } while(lexer.ConsumeIf(TokenType::Coma));
    }

    if (Token token = lexer.Consume(); token.type != TokenType::RPar)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting closing parenthesis", token.name), token.location };
    
    if (Token token = lexer.Peek(); token.type != TokenType::LBracket)
        throw Exception{ std::format("Unexpected token \'{}\'.", token.name), token.location };

    std::unique_ptr<ASTStatement> body = ParseCompoundStatement(lexer);

    return FillASTStatementDebugInformation(std::make_unique<ASTTemplateFunctionDeclaration>(
        typeInfo, functionIdentifierToken.name, std::move(parameters), std::move(arguments), std::move(body)
    ), functionIdentifierToken);
}

std::unique_ptr<VCL::ASTReturnStatement> VCL::Parser::ParseReturnStatement(Lexer& lexer) {
    Token returnToken = lexer.Consume(); //RETURN
    std::unique_ptr<ASTExpression> expression = ParseExpression(lexer);
    return FillASTStatementDebugInformation(std::make_unique<ASTReturnStatement>(std::move(expression)), returnToken);
}

std::unique_ptr<VCL::ASTIfStatement> VCL::Parser::ParseIfStatement(Lexer& lexer) {
    Token ifToken = lexer.Consume(); //Consume if token

    if (Token token = lexer.Consume(); token.type != TokenType::LPar)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting opening parenthesis", token.name), token.location };

    std::unique_ptr<ASTExpression> condition = ParseExpression(lexer);

    if (Token token = lexer.Consume(); token.type != TokenType::RPar)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting closing parenthesis", token.name), token.location };

    std::unique_ptr<ASTStatement> thenStatement = nullptr;

    if (lexer.Peek().type == TokenType::LBracket)
        thenStatement = ParseCompoundStatement(lexer);
    else
        thenStatement = ParseStatement(lexer);

    std::unique_ptr<ASTStatement> elseStatement = nullptr;

    if (lexer.Peek().type == TokenType::Else) {
        lexer.Consume();
        if (lexer.Peek().type == TokenType::LBracket)
            elseStatement = ParseCompoundStatement(lexer);
        else
            elseStatement = ParseStatement(lexer);
    }

    return FillASTStatementDebugInformation(std::make_unique<ASTIfStatement>(std::move(condition), 
        std::move(thenStatement), std::move(elseStatement)), ifToken);
}

std::unique_ptr<VCL::ASTForStatement> VCL::Parser::ParseForStatement(Lexer& lexer) {
    Token forToken = lexer.Consume(); //Consume for token

    if (Token token = lexer.Consume(); token.type != TokenType::LPar)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting opening parenthesis", token.name), token.location };

    std::unique_ptr<ASTStatement> start = ParseStatement(lexer);
    std::unique_ptr<ASTExpression> condition = ParseExpression(lexer);

    if (Token token = lexer.Consume(); token.type != TokenType::Semicolon)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting semicolon", token.name), token.location };

    std::unique_ptr<ASTStatement> end = ParseStatement(lexer, true);

    if (Token token = lexer.Consume(); token.type != TokenType::RPar)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting closing parenthesis", token.name), token.location };

    std::unique_ptr<ASTStatement> thenStatement = nullptr;

    if (lexer.Peek().type == TokenType::LBracket)
        thenStatement = ParseCompoundStatement(lexer);
    else
        thenStatement = ParseStatement(lexer);

    return FillASTStatementDebugInformation(std::make_unique<ASTForStatement>(std::move(start), 
        std::move(condition), std::move(end), std::move(thenStatement)), forToken);
}

std::unique_ptr<VCL::ASTWhileStatement> VCL::Parser::ParseWhileStatement(Lexer& lexer) {
    Token whileToken = lexer.Consume(); //Consume while token

    if (Token token = lexer.Consume(); token.type != TokenType::LPar)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting opening parenthesis", token.name), token.location };

    std::unique_ptr<ASTExpression> condition = ParseExpression(lexer);

    if (Token token = lexer.Consume(); token.type != TokenType::RPar)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting closing parenthesis", token.name), token.location };
    
    std::unique_ptr<ASTStatement> thenStatement = nullptr;

    if (lexer.Peek().type == TokenType::LBracket)
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

std::unique_ptr<VCL::ASTExpression> VCL::Parser::ParseExpression(Lexer& lexer) {
    std::unique_ptr<ASTExpression> primary = ParsePrefixExpression(lexer);
    return ParseBinaryExpression(lexer, std::move(primary), 0);
}

VCL::Operator GetBinaryOperator(VCL::TokenType type) {
    switch (type)
    {
    case VCL::TokenType::Asterisk:
        return VCL::Operator{  VCL::Operator::ID::Mul, VCL::Operator::Kind::Arithmetic, VCL::Operator::Associativity::Left, 12 };
    case VCL::TokenType::Slash:
        return VCL::Operator{  VCL::Operator::ID::Div, VCL::Operator::Kind::Arithmetic, VCL::Operator::Associativity::Left, 12 };
    case VCL::TokenType::Remainder:
        return VCL::Operator{  VCL::Operator::ID::Remainder, VCL::Operator::Kind::Arithmetic, VCL::Operator::Associativity::Left, 12 };
    case VCL::TokenType::Plus:
        return VCL::Operator{  VCL::Operator::ID::Add, VCL::Operator::Kind::Arithmetic, VCL::Operator::Associativity::Left, 11 };
    case VCL::TokenType::Minus:
        return VCL::Operator{  VCL::Operator::ID::Sub, VCL::Operator::Kind::Arithmetic, VCL::Operator::Associativity::Left, 11 };
    case VCL::TokenType::Greater:
        return VCL::Operator{  VCL::Operator::ID::Greater, VCL::Operator::Kind::Comparison, VCL::Operator::Associativity::Left, 9 };
    case VCL::TokenType::Less:
        return VCL::Operator{  VCL::Operator::ID::Less, VCL::Operator::Kind::Comparison, VCL::Operator::Associativity::Left, 9 };
    case VCL::TokenType::GreaterEqual:
        return VCL::Operator{  VCL::Operator::ID::GreaterEqual, VCL::Operator::Kind::Comparison, VCL::Operator::Associativity::Left, 9 };
    case VCL::TokenType::LessEqual:
        return VCL::Operator{  VCL::Operator::ID::LessEqual, VCL::Operator::Kind::Comparison, VCL::Operator::Associativity::Left, 9 };
    case VCL::TokenType::Equal:
        return VCL::Operator{  VCL::Operator::ID::Equal, VCL::Operator::Kind::Comparison, VCL::Operator::Associativity::Left, 8 };
    case VCL::TokenType::NotEqual:
        return VCL::Operator{  VCL::Operator::ID::NotEqual, VCL::Operator::Kind::Comparison, VCL::Operator::Associativity::Left, 8 };
    case VCL::TokenType::LogicalAnd:
        return VCL::Operator{  VCL::Operator::ID::LogicalAnd, VCL::Operator::Kind::Logical, VCL::Operator::Associativity::Left, 4 };
    case VCL::TokenType::LogicalOr:
        return VCL::Operator{  VCL::Operator::ID::LogicalOr, VCL::Operator::Kind::Logical, VCL::Operator::Associativity::Left, 3 };
    case VCL::TokenType::Assignment:
        return VCL::Operator{  VCL::Operator::ID::Add, VCL::Operator::Kind::Assignment, VCL::Operator::Associativity::Right, 1 };
    default:
        return VCL::Operator{};
    }
}


VCL::Operator GetPrefixOperator(VCL::TokenType type) {
    switch (type)
    {
    case VCL::TokenType::Plus:
        return VCL::Operator{  VCL::Operator::ID::Plus, VCL::Operator::Kind::Arithmetic, VCL::Operator::Associativity::Right, 13 };
    case VCL::TokenType::Minus:
        return VCL::Operator{  VCL::Operator::ID::Minus, VCL::Operator::Kind::Arithmetic, VCL::Operator::Associativity::Right, 13 };
    case VCL::TokenType::Increment:
        return VCL::Operator{  VCL::Operator::ID::PreIncrement, VCL::Operator::Kind::Arithmetic, VCL::Operator::Associativity::Right, 13 };
    case VCL::TokenType::Decrement:
        return VCL::Operator{  VCL::Operator::ID::PreDecrement, VCL::Operator::Kind::Arithmetic, VCL::Operator::Associativity::Right, 13 };
    case VCL::TokenType::ExclmMark:
        return VCL::Operator{  VCL::Operator::ID::Not, VCL::Operator::Kind::Logical, VCL::Operator::Associativity::Right, 13 };
    default:
        return VCL::Operator{};
    }
}

VCL::Operator GetPostfixOperator(VCL::TokenType type) {
    switch (type)
    {
    case VCL::TokenType::Increment:
        return VCL::Operator{  VCL::Operator::ID::PostIncrement, VCL::Operator::Kind::Arithmetic, VCL::Operator::Associativity::Left, 14 };
    case VCL::TokenType::Decrement:
        return VCL::Operator{  VCL::Operator::ID::PostDecrement, VCL::Operator::Kind::Arithmetic, VCL::Operator::Associativity::Left, 14 };
    case VCL::TokenType::Dot:
        return VCL::Operator{  VCL::Operator::ID::FieldAccess, VCL::Operator::Kind::FieldAccess, VCL::Operator::Associativity::Left, 14 };
    case VCL::TokenType::LSquareBracket:
        return VCL::Operator{  VCL::Operator::ID::Subscript, VCL::Operator::Kind::Subscript, VCL::Operator::Associativity::Left, 14 };
    default:
        return VCL::Operator{};
    }
}

std::unique_ptr<VCL::ASTExpression> VCL::Parser::ParseBinaryExpression(Lexer& lexer, std::unique_ptr<ASTExpression> lhs, int precedence) {
    Token lookaheadToken = lexer.Peek();
    Operator lookahead = GetBinaryOperator(lexer.Peek().type);
    while (lookahead.precedence >= precedence) {
        Token opToken = lookaheadToken;
        Operator op = lookahead;
        lexer.Consume(); //Consume operator
        std::unique_ptr<ASTExpression> rhs = ParsePrefixExpression(lexer);
        lookaheadToken = lexer.Peek();
        lookahead = GetBinaryOperator(lexer.Peek().type);
        while (lookahead.precedence > op.precedence || 
            (lookahead.precedence == op.precedence && lookahead.associativity == Operator::Associativity::Right)) {
            rhs = ParseBinaryExpression(lexer, std::move(rhs), op.precedence + (lookahead.precedence > op.precedence ? 1 : 0));
            lookaheadToken = lexer.Peek();
            lookahead = GetBinaryOperator(lexer.Peek().type);
        }
        SourceLocation newLocation = lhs->location.Combine(rhs->location);
        switch (op.kind) {
            case Operator::Kind::Arithmetic:
                lhs = std::make_unique<ASTBinaryArithmeticExpression>(op.id, std::move(lhs), std::move(rhs));
                break;
            case Operator::Kind::Logical:
                lhs = std::make_unique<ASTBinaryLogicalExpression>(op.id, std::move(lhs), std::move(rhs));
                break;
            case Operator::Kind::Comparison:
                lhs = std::make_unique<ASTBinaryComparisonExpression>(op.id, std::move(lhs), std::move(rhs));
                break;
            case Operator::Kind::Assignment:
                lhs = std::make_unique<ASTAssignmentExpression>(op.id, std::move(lhs), std::move(rhs));
                break;
            default:
                throw std::runtime_error{ "Error parsing binary expression." };
        }
        lhs->location = newLocation;
    }
    return std::move(lhs);
}

std::unique_ptr<VCL::ASTExpression> VCL::Parser::ParsePrefixExpression(Lexer& lexer) {
    Token opToken = lexer.Peek();
    Operator op = GetPrefixOperator(lexer.Peek().type);
    if (op.precedence == -1)
        return ParsePostfixExpression(lexer);
    lexer.Consume(); //Consume operator
    std::unique_ptr<ASTExpression> expression = ParsePrefixExpression(lexer);
    SourceLocation newLocation = opToken.location.Combine(expression->location);
    switch (op.kind) {
        case Operator::Kind::Arithmetic:
            expression = std::make_unique<ASTPrefixArithmeticExpression>(op.id, std::move(expression));
            break;
        case Operator::Kind::Logical:
            expression = std::make_unique<ASTPrefixLogicalExpression>(op.id, std::move(expression));
            break;
        default:
            throw std::runtime_error{ "Error parsing prefix expression." };
    }
    expression->location = newLocation;
    return std::move(expression);
}

std::unique_ptr<VCL::ASTExpression> VCL::Parser::ParsePostfixExpression(Lexer& lexer) {
    std::unique_ptr<ASTExpression> expression = ParsePrimaryExpression(lexer);
    Token opToken = lexer.Peek();
    Operator op = GetPostfixOperator(lexer.Peek().type);
    while (op.precedence != -1) {
        lexer.Consume(); //Consume operator
        SourceLocation newLocation = opToken.location.Combine(expression->location);
        switch (op.kind) {
            case Operator::Kind::Arithmetic:
                expression = std::make_unique<ASTPostfixArithmeticExpression>(op.id, std::move(expression));
                break;
            case Operator::Kind::FieldAccess:
                {
                    Token fieldNameToken = lexer.Consume();
                    if (fieldNameToken.type != TokenType::Identifier)
                        throw Exception{ std::format("Unexpected token \'{}\'. Expecting identifier.", 
                            fieldNameToken.name), fieldNameToken.location };
                    expression = std::make_unique<ASTFieldAccessExpression>(std::move(expression), fieldNameToken.name);
                }
                break;
            case Operator::Kind::Subscript:
                {
                    std::unique_ptr<ASTExpression> index = ParseExpression(lexer);
                    if (Token token = lexer.Consume(); token.type != TokenType::RSquareBracket)
                        throw Exception{ std::format("Unexpected token \'{}\'. Expecting right square bracket.", 
                            token.name), token.location };
                    expression = std::make_unique<ASTSubscriptExpression>(std::move(expression), std::move(index));
                }
                break;
            default:
                throw std::runtime_error{ "Error parsing prefix expression." };
        }
        expression->location = newLocation;
        op = GetPostfixOperator(lexer.Peek().type);
        opToken = lexer.Peek();
    }
    return std::move(expression);
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

    if (IsTypeInfoToken(lexer)) {
        std::shared_ptr<TypeInfo> typeInfo = ParseTypeInfo(lexer);
        expression = ParseVariableDeclaration(lexer, typeInfo);
    } else if (currentToken.type == TokenType::Identifier) {
        if (Token nextToken = lexer.Peek(1); nextToken.type == TokenType::Less) {
            expression = TryParseTemplatedFunctionCall(lexer);
            if (!expression)
                expression = ParseVariableExpression(lexer);
        } else if (nextToken.type == TokenType::LPar) {
            expression = ParseFunctionCall(lexer);
        } else {
            expression = ParseVariableExpression(lexer);
        }
    } else if (IsLiteralToken(currentToken)) {
        expression = ParseLiteralExpression(lexer);
    } else if (currentToken.type == TokenType::LPar) {
        expression = ParseParentExpression(lexer);
    }

    if (expression == nullptr)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting primary expression", currentToken.name), 
            currentToken.location };
    
    return expression;
}

std::unique_ptr<VCL::ASTExpression> VCL::Parser::ParseParentExpression(Lexer& lexer) {
    lexer.Consume(); //LPAR
    std::unique_ptr<ASTExpression> expression = ParseExpression(lexer);
    Token closingParenthesisToken = lexer.Consume();
    if (closingParenthesisToken.type != TokenType::RPar) 
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting closing parenthesis.", closingParenthesisToken.name), 
            closingParenthesisToken.location };
    return std::move(expression);
}

std::unique_ptr<VCL::ASTLiteralExpression> VCL::Parser::ParseLiteralExpression(Lexer& lexer) {
    Token literalToken = lexer.Consume();
    std::string literalStr{ literalToken.name };
    switch (literalToken.type)
    {
    case TokenType::LiteralFloat:
        return FillASTStatementDebugInformation(std::make_unique<ASTLiteralExpression>(std::stof(literalStr)), literalToken);
    case TokenType::LiteralInt:
        return FillASTStatementDebugInformation(std::make_unique<ASTLiteralExpression>(std::stoi(literalStr)), literalToken);
    default:
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting literal expression.", literalToken.name), literalToken.location };
    }
}

std::unique_ptr<VCL::ASTVariableExpression> VCL::Parser::ParseVariableExpression(Lexer& lexer) {
    Token variableIdentifierToken = lexer.Consume();
    if (variableIdentifierToken.type != TokenType::Identifier)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting identifier.", variableIdentifierToken.name), variableIdentifierToken.location };
    return FillASTStatementDebugInformation(std::make_unique<ASTVariableExpression>(variableIdentifierToken.name), variableIdentifierToken);
}

std::unique_ptr<VCL::ASTVariableDeclaration> VCL::Parser::ParseVariableDeclaration(Lexer& lexer, std::shared_ptr<TypeInfo> typeInfo) {
    Token variableIdentifierToken = lexer.Consume();
    if (variableIdentifierToken.type != TokenType::Identifier)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting identifier.", variableIdentifierToken.name), variableIdentifierToken.location };
    std::unique_ptr<ASTExpression> expression = nullptr;
    if (lexer.Peek().type == TokenType::Assignment) {
        lexer.Consume();
        expression = ParseExpression(lexer);
    }
    return FillASTStatementDebugInformation(
            std::make_unique<ASTVariableDeclaration>(typeInfo, variableIdentifierToken.name, std::move(expression)), variableIdentifierToken);
}

std::unique_ptr<VCL::ASTFunctionCall> VCL::Parser::ParseFunctionCall(Lexer& lexer) {
    Token functionIdentifierToken = lexer.Consume();
    if (functionIdentifierToken.type != TokenType::Identifier)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting identifier.", functionIdentifierToken.name), functionIdentifierToken.location };
    lexer.Consume(); //LPAR

    std::vector<std::unique_ptr<ASTExpression>> arguments{};

    if (lexer.Peek().type != TokenType::RPar) {
        do {
            std::unique_ptr<ASTExpression> expression = ParseExpression(lexer);
            arguments.emplace_back(std::move(expression));
        } while (lexer.ConsumeIf(TokenType::Coma));
    }

    if (Token token = lexer.Consume(); token.type != TokenType::RPar)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting closing parenthesis", token.name), token.location };

    return FillASTStatementDebugInformation(std::make_unique<ASTFunctionCall>(functionIdentifierToken.name, std::move(arguments)), functionIdentifierToken);
}

std::unique_ptr<VCL::ASTFunctionCall> VCL::Parser::TryParseTemplatedFunctionCall(Lexer& lexer) {
    uint32_t cursor = lexer.GetCursor();

    Token functionIdentifierToken = lexer.Consume();
    lexer.Consume(); //Less

    std::vector<std::shared_ptr<TemplateArgument>> templateArguments{};

    try {
        if (Token token = lexer.Peek(); token.type == TokenType::Greater) {
            lexer.Consume();
        } else {
            do {
                templateArguments.push_back(ParseTemplateArgument(lexer));
            } while (lexer.ConsumeIf(TokenType::Coma));

            if (Token token = lexer.Consume(); token.type != TokenType::Greater)
                throw Exception{ std::format("Unexpected token \'{}\'.", token.name), token.location };
        }
    } catch (std::exception& e) {
        lexer.SetCursor(cursor);
        return nullptr;
    }
    
    if (Token token = lexer.Consume(); token.type != TokenType::LPar)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting function call operator.", token.name), token.location };

    std::vector<std::unique_ptr<ASTExpression>> arguments{};

    if (lexer.Peek().type != TokenType::RPar) {
        do {
            std::unique_ptr<ASTExpression> expression = ParseExpression(lexer);
            if (expression == nullptr)
                return nullptr;
            arguments.emplace_back(std::move(expression));
        } while (lexer.ConsumeIf(TokenType::Coma));
    }

    if (Token token = lexer.Consume(); token.type != TokenType::RPar)
        throw Exception{ std::format("Unexpected token \'{}\'. Expecting closing parenthesis", token.name), token.location };

    return FillASTStatementDebugInformation(std::make_unique<ASTFunctionCall>(
        functionIdentifierToken.name, std::move(arguments), std::move(templateArguments)), functionIdentifierToken);
}

std::shared_ptr<VCL::TypeInfo> VCL::Parser::ParseTypeInfo(Lexer& lexer) {
    std::shared_ptr<TypeInfo> typeInfo = std::make_shared<TypeInfo>();
    bool complete = false;
    Token lastTypeInfoToken{ TokenType::Undefined };
    while (!complete) {
        Token currentTypeInfoToken = lexer.Consume();
        switch (currentTypeInfoToken.type) {
            //Qualifier(s)
            case TokenType::In:
                typeInfo->qualifiers |= TypeInfo::QualifierFlag::In;
                break;
            case TokenType::Out:
                typeInfo->qualifiers |= TypeInfo::QualifierFlag::Out;
                break;
            case TokenType::Const:
                typeInfo->qualifiers |= TypeInfo::QualifierFlag::Const;
                break;
            //TypeName
            case TokenType::Float:
                typeInfo->type = TypeInfo::TypeName::Float;
                complete = true;
                break;
            case TokenType::Bool:
                typeInfo->type = TypeInfo::TypeName::Bool;
                complete = true;
                break;
            case TokenType::Int:
                typeInfo->type = TypeInfo::TypeName::Int;
                complete = true;
                break;
            case TokenType::Void:
                typeInfo->type = TypeInfo::TypeName::Void;
                complete = true;
                break;
            case TokenType::VectorFloat:
                typeInfo->type = TypeInfo::TypeName::VectorFloat;
                complete = true;
                break;
            case TokenType::VectorBool:
                typeInfo->type = TypeInfo::TypeName::VectorBool;
                complete = true;
                break;
            case TokenType::VectorInt:
                typeInfo->type = TypeInfo::TypeName::VectorInt;
                complete = true;
                break;
            case TokenType::Array:
                typeInfo->type = TypeInfo::TypeName::Array;
                complete = true;
                break;
            case TokenType::Span:
                typeInfo->type = TypeInfo::TypeName::Span;
                complete = true;
                break;
            case TokenType::Identifier:
                typeInfo->type = TypeInfo::TypeName::Custom;
                typeInfo->name = currentTypeInfoToken.name;
                complete = true;
                break;
            default:
                if (lastTypeInfoToken.type != TokenType::Undefined)
                    throw Exception{ std::format("Unexpected token \'{}\' after \'{}\'. Incomplete type error.",
                        currentTypeInfoToken.name, lastTypeInfoToken.name), currentTypeInfoToken.location };
                else
                    throw Exception{ std::format("Unexpected token \'{}\'. Incomplete type error.",
                        currentTypeInfoToken.name), currentTypeInfoToken.location };
        }
        lastTypeInfoToken = currentTypeInfoToken;
    }

    //Template
    if (lexer.ConsumeIf(TokenType::Less)) {
        do {
            typeInfo->arguments.push_back(ParseTemplateArgument(lexer));
        } while (lexer.ConsumeIf(TokenType::Coma));

        if (Token token = lexer.Consume(); token.type != TokenType::Greater)
            throw Exception{ std::format("Unexpected token \'{}\'.", token.name), token.location };
    }

    return typeInfo;
}

std::shared_ptr<VCL::TemplateArgument> VCL::Parser::ParseTemplateArgument(Lexer& lexer) {
    std::shared_ptr<TemplateArgument> argument = std::make_shared<TemplateArgument>();
    Token currentToken = lexer.Peek();
    std::string currentTokenStr{ currentToken.name };
    switch (currentToken.type) {
        //TypeName
        case TokenType::LiteralInt:
            argument->type = TemplateArgument::TemplateValueType::Int;
            argument->intValue = std::stoi(currentTokenStr);
            lexer.Consume();
            break;
        default:
            argument->type = TemplateArgument::TemplateValueType::Typename;
            argument->typeInfo = ParseTypeInfo(lexer);
            break;
    }
    return argument;
}

std::unique_ptr<VCL::Parser> VCL::Parser::Create(std::shared_ptr<Logger> logger) {
    std::unique_ptr<VCL::Parser> parser = std::make_unique<Parser>();
    parser->SetLogger(logger);
    return std::move(parser);
}