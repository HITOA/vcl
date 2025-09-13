#include <VCL/Parse/Parser.hpp>

#include <VCL/AST/Operator.hpp>

#define NEXT_TOKEN_N(n) if (!NextToken(n)) return nullptr
#define NEXT_TOKEN() if (!NextToken()) return nullptr
#define GET_TOKEN_N(token, n) token = GetToken(n); if (token == nullptr) return nullptr
#define GET_TOKEN(token) token = GetToken(); if (token == nullptr) return nullptr
#define EXPECT_TOKEN_N(token, kind, n) token = ExpectToken(kind, n, __FILE__, __func__, __LINE__); if (token == nullptr) return nullptr
#define EXPECT_TOKEN(token, kind) token = ExpectToken(kind, 0, __FILE__, __func__, __LINE__); if (token == nullptr) return nullptr


VCL::Parser::Parser(TokenStream& stream, Sema& sema, CompilerContext& cc) : stream{ stream }, sema{ sema }, cc{ cc } {

}


VCL::Token* VCL::Parser::ExpectToken(VCL::TokenKind kind, uint32_t n, const char* file, const char* func, uint32_t line) {
    Token* token = stream.GetTok(n);
    if (!token)
        return nullptr;
    if (token->kind != kind) {
        cc.GetDiagnosticReporter().Error(Diagnostic::UnexpectedToken, std::string{ token->range.start.GetPtr(), token->range.end.GetPtr() })
            .AddHint(DiagnosticHint{ token->range })
            .SetCompilerInfo(file, func, line)
            .Report();
        return nullptr;
    }
    return token;
}

bool VCL::Parser::Parse() {
    while (GetToken()->kind != TokenKind::EndOfFile) {
        Decl* decl = ParseTopLevelDecl();
        if (decl == nullptr)
            return false;
    }
    return true;
}

VCL::Decl* VCL::Parser::ParseTopLevelDecl() {
    Token* token;
    GET_TOKEN(token);
    switch (token->kind) {
        default:
            if (VarDecl* decl = ParseVarDecl(TokenKind::Semicolon); decl != nullptr)
                return decl;
            return nullptr;
    }
}

VCL::VarDecl* VCL::Parser::ParseVarDecl(TokenKind expectedEndTokenKind) {
    SourceRange declRange = GetToken()->range;
    VarDecl::VarAttrBitfield attr{ 0 };
    if (auto r = ParseVarAttrBitfield(); r.has_value())
        attr = *r;
    else
        return nullptr;
    QualType declType = ParseQualType().value;
    if (declType.GetType() == nullptr)
        return nullptr;
    Token* token;
    EXPECT_TOKEN(token, TokenKind::Identifier);
    IdentifierInfo* identifier = token->identifier;
    declRange.end = token->range.end;
    NEXT_TOKEN();
    GET_TOKEN(token);
    Expr* initializer = nullptr;
    if (token->kind == TokenKind::Equal) {
        NEXT_TOKEN();
        initializer = ParseExpression();
        if (!initializer)
            return nullptr;
    }
    if (expectedEndTokenKind != TokenKind::Unknown) {
        EXPECT_TOKEN(token, expectedEndTokenKind);
        NEXT_TOKEN();
    }
    VarDecl* decl = sema.ActOnVarDecl(declType, identifier, attr, initializer, declRange);
    return decl;
}

VCL::VarDecl* VCL::Parser::TryParseVarDecl(TokenKind expectedEndTokenKind) {
    TentativeParsingGuard tp{ this };
    VarDecl* decl = ParseVarDecl(expectedEndTokenKind);
    if (decl != nullptr)
        tp.Commit();
    return decl;
}

std::optional<VCL::VarDecl::VarAttrBitfield> VCL::Parser::ParseVarAttrBitfield() {
    VarDecl::VarAttrBitfield attr{ 0 };

    Token* token = GetToken();
    if (!token)
        return {};

    while (token) {
        switch (token->kind) {
            case TokenKind::Keyword_in:
                attr.hasInAttribute = 1;
                token = NextAndGetToken();
                break;
            case TokenKind::Keyword_out:
                attr.hasOutAttribute = 1;
                token = NextAndGetToken();
                break;
            case TokenKind::Keyword_inout:
                attr.hasInAttribute = 1;
                attr.hasOutAttribute = 1;
                token = NextAndGetToken();
                break;
            default:
                token = nullptr;
                break;
        }
    }

    return attr;
}

VCL::WithFullLoc<VCL::QualType> VCL::Parser::ParseQualType() {
    Token* token = GetToken();
    if (!token)
        return WithFullLoc<QualType>{};
    SourceRange range = token->range;
    Qualifier qualifiers = Qualifier::None;
    if (!token)
        return WithFullLoc<QualType>{};
        
    if (token->kind == TokenKind::Identifier && token->identifier->GetTokenKind() == TokenKind::Keyword_const) {
        qualifiers = qualifiers | Qualifier::Const;
        if (!NextToken())
            return WithFullLoc<QualType>{};
    }
    WithFullLoc<Type*> type = ParseType();
    if (!type.value)
        return WithFullLoc<QualType>{};
    range.end = type.range.end;
    return WithFullLoc<QualType>{ sema.ActOnQualType(type.value, qualifiers, range), range };
}

VCL::WithFullLoc<VCL::QualType> VCL::Parser::TryParseQualType() {
    TentativeParsingGuard tp{ this };
    WithFullLoc<QualType> r = ParseQualType();
    if (r.value.GetType() != nullptr)
        tp.Commit();
    return r;
}

VCL::WithFullLoc<VCL::Type*> VCL::Parser::ParseType() {
    Token* token = ExpectToken(TokenKind::Identifier, 0, __FILE__, __func__, __LINE__);
    if (!token)
        return WithFullLoc<Type*>{};

    SourceRange range = token->range;
    IdentifierInfo* identifier = token->identifier;

    if (!NextToken())
        return WithFullLoc<Type*>{};
    
    TemplateArgumentList* list = nullptr;
    token = GetToken();
    if (!token)
        return WithFullLoc<Type*>{};

    if (token->kind == TokenKind::Lesser) {
        list = ParseTemplateArgumentList();
        if (list == nullptr)
            return WithFullLoc<Type*>{};
        range.end = list->GetSourceRange().end;
    }
    
    token = GetToken();
    if (!token)
        return WithFullLoc<Type*>{};

    return WithFullLoc<Type*>{ sema.ActOnType(identifier, list, range), range };
}

VCL::WithFullLoc<VCL::Type*> VCL::Parser::TryParseType() {
    TentativeParsingGuard tp{ this };
    WithFullLoc<Type*> r = ParseType();
    if (r.value != nullptr)
        tp.Commit();
    return r;
}

VCL::TemplateArgumentList* VCL::Parser::ParseTemplateArgumentList() {
    ParserFlagGuard flagGuard{ this, ParserFlag::OnTemplateArgument };

    llvm::SmallVector<TemplateArgument> args{};
    SourceRange range{};

    Token* token;
    EXPECT_TOKEN(token, TokenKind::Lesser);
    range.start = token->range.start;
    NEXT_TOKEN();
    do {
        if (!args.empty()) {
            EXPECT_TOKEN(token, TokenKind::Coma);
            NEXT_TOKEN();
        }
        if (WithFullLoc<QualType> type = TryParseQualType(); type.value.GetAsOpaquePtr() != 0) {
            args.emplace_back(type.value).SetSourceRange(type.range);
        } else if (Expr* expr = TryParseExpression(); expr != nullptr) {
            if (ConstantValue* value = expr->GetConstantValue(); 
                value != nullptr && value->GetConstantValueClass() == ConstantValue::ConstantScalarClass) {
                ConstantScalar* scalar = (ConstantScalar*)value;
                args.emplace_back(*scalar).SetSourceRange(expr->GetSourceRange());
            } else {
                args.emplace_back(expr).SetSourceRange(expr->GetSourceRange());
            }
        } else {
            GET_TOKEN(token);
            cc.GetDiagnosticReporter().Error(Diagnostic::WrongTemplateArgument)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .AddHint(DiagnosticHint{ SourceRange{ token->range.start, token->range.start } })
                .Report();
            return nullptr;
        }
        GET_TOKEN(token);
    } while (token->kind != TokenKind::Greater);
    range.end = token->range.end;
    NEXT_TOKEN();

    return sema.ActOnTemplateArgumentList(args, range);
}

VCL::Expr* VCL::Parser::ParseExpression() {
    Expr* expr = ParsePrefixExpression();
    if (expr == nullptr)
        return nullptr;
    return ParseBinaryExpression(expr, 0);
}

VCL::Expr* VCL::Parser::TryParseExpression() {
    TentativeParsingGuard tp{ this };
    Expr* expr = ParseExpression();
    if (expr != nullptr)
        tp.Commit();
    return expr;
}

VCL::Expr* VCL::Parser::ParseBinaryExpression(Expr* lhs, int precedence) {
    Token* token;
    bool onTemplateArgument = (uint32_t)flags & (uint32_t)ParserFlag::OnTemplateArgument;
    GET_TOKEN(token);
    BinaryOperator lookaheadOp = GetBinaryOperator(*token, onTemplateArgument);
    while (lookaheadOp.precedence >= precedence) {
        BinaryOperator op = lookaheadOp;
        NEXT_TOKEN();
        GET_TOKEN(token);
        Expr* rhs = ParsePrefixExpression();
        if (!rhs)
            return nullptr;
        
        lookaheadOp = GetBinaryOperator(*token, onTemplateArgument);
        while(lookaheadOp.precedence > op.precedence || 
            (lookaheadOp.precedence == op.precedence && lookaheadOp.associativity == OperatorAssociativity::Right)) {
            rhs = ParseBinaryExpression(rhs, op.precedence + (lookaheadOp.precedence > op.precedence ? 1 : 0));
            if (!rhs)
                return nullptr;
            GET_TOKEN(token);
            lookaheadOp = GetBinaryOperator(*token, onTemplateArgument);
        }
        SourceRange range{ lhs->GetSourceRange().start, rhs->GetSourceRange().end };
        lhs = sema.ActOnBinaryExpr(lhs, rhs, op);
        if (!lhs)
            return nullptr;
    }
    return lhs;
}

VCL::Expr* VCL::Parser::ParsePrefixExpression() {
    return ParsePostfixExpression();
}

VCL::Expr* VCL::Parser::TryParsePrefixExpression() {
    TentativeParsingGuard tp{ this };
    Expr* expr = ParsePrefixExpression();
    if (expr != nullptr)
        tp.Commit();
    return expr;
}

VCL::Expr* VCL::Parser::ParsePostfixExpression() {
    return ParsePrimaryExpression();
}

VCL::Expr* VCL::Parser::ParsePrimaryExpression() {
    Token* token;
    GET_TOKEN(token);
    switch (token->kind) {
        case TokenKind::NumericConstant:
            return ParseNumericConstantExpr();
        case TokenKind::Identifier:
            return ParseIdentifierExpr();
        case TokenKind::LeftPar:
        default:
            cc.GetDiagnosticReporter().Error(Diagnostic::UnexpectedToken, std::string{ token->range.start.GetPtr(), token->range.end.GetPtr() })
                .AddHint(DiagnosticHint{ token->range })
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
    }
}

VCL::Expr* VCL::Parser::ParseNumericConstantExpr() {
    Token* token;
    EXPECT_TOKEN(token, TokenKind::NumericConstant);
    SourceRange range = token->range;
    Expr* expr = sema.ActOnNumericConstant(token);
    NEXT_TOKEN();
    return expr;
}

VCL::Expr* VCL::Parser::ParseIdentifierExpr() {
    Token* token;
    EXPECT_TOKEN(token, TokenKind::Identifier);
    SourceRange range = token->range;
    Expr* expr = sema.ActOnIdentifierExpr(token->identifier, range);
    NEXT_TOKEN();
    return expr;
}