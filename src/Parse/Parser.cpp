#include <VCL/Parse/Parser.hpp>

#include <VCL/AST/Operator.hpp>
#include <VCL/AST/ExprEvaluator.hpp>

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
    TokenKind ckind = token->kind;
    if (token->kind == TokenKind::Identifier && kind != TokenKind::Identifier)
        ckind = token->identifier->GetTokenKind();
    if (ckind != kind) {
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
    TokenKind kind = token->kind;
    if (token->kind == TokenKind::Identifier)
        kind = token->identifier->GetTokenKind();
    switch (kind) {
        case TokenKind::Keyword_struct:
            return ParseAnyRecordDecl();
        default:
            return ParseFunctionDeclOrVarDecl();
    }
}

VCL::Decl* VCL::Parser::ParseRecordLevelDecl() {
    return ParseFieldDecl();
}

VCL::Decl* VCL::Parser::ParseFunctionDeclOrVarDecl() {
    Token* token;
    GET_TOKEN(token);
    SourceRange range = token->range;
    auto attr = ParseVarAttrBitfield();
    if (!attr.has_value())
        return nullptr;
    WithFullLoc<QualType> type = ParseQualType();
    if (type.value.GetAsOpaquePtr() == 0)
        return nullptr;
    EXPECT_TOKEN(token, TokenKind::Identifier);
    IdentifierInfo* identifier = token->identifier;
    range.end = token->range.end;
    NEXT_TOKEN();
    GET_TOKEN(token);
    switch (token->kind) {
        case TokenKind::Equal:
        case TokenKind::Semicolon:
            return ParseEndVarDecl(attr.value(), type.value, identifier, range);
        case TokenKind::LeftPar:
            return ParseEndFunctionDecl(type.value, identifier, range);
        default:
            cc.GetDiagnosticReporter().Error(Diagnostic::UnexpectedToken, std::string{ token->range.start.GetPtr(), token->range.end.GetPtr() })
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .AddHint(DiagnosticHint{ token->range })
                .Report();
            return nullptr;
    }
}

VCL::CompoundStmt* VCL::Parser::ParseCompoundStmt() {
    llvm::SmallVector<Stmt*> stmts{};
    
    Token* token;
    EXPECT_TOKEN(token, TokenKind::LeftBrace);
    SourceRange range = token->range;
    NEXT_TOKEN();
    GET_TOKEN(token);

    while (token->kind != TokenKind::RightBrace) {
        GET_TOKEN(token);
        TokenKind kind = token->kind;
        if (token->kind == TokenKind::Identifier)
            kind = token->identifier->GetTokenKind();
        switch (kind) {
            case TokenKind::Keyword_return: {
                ReturnStmt* stmt = ParseReturnStmt();
                if (!stmt)
                    return nullptr;
                stmts.push_back(stmt);
                break;
            }
            default: {
                VarDecl* decl = TryParseVarDecl();
                if (!decl) {
                    Expr* expr = ParseExpression();
                    if (!expr)
                        return nullptr;
                    EXPECT_TOKEN(token, TokenKind::Semicolon);
                    NEXT_TOKEN();
                    stmts.push_back(expr);
                    break;
                }
                DeclStmt* stmt = sema.ActOnDeclStmt(decl, decl->GetSourceRange());
                if (!stmt)
                    return nullptr;
                stmts.push_back(stmt);
                break;
            }
        }
        GET_TOKEN(token);
    }
    range.end = token->range.end;
    NEXT_TOKEN();

    return sema.ActOnCompoundStmt(stmts, range);
}

VCL::NamedDecl* VCL::Parser::ParseAnyRecordDecl() {
    SourceRange range;
    Token* token;
    EXPECT_TOKEN(token, TokenKind::Keyword_struct);
    range = token->range;
    NEXT_TOKEN();
    EXPECT_TOKEN(token, TokenKind::Identifier);
    IdentifierInfo* identifier = token->identifier;
    range.end = token->range.end;
    NEXT_TOKEN();
    GET_TOKEN(token);
    TemplateParameterList* params = nullptr;
    if (token->kind == TokenKind::Lesser) {
        params = ParseTemplateParameterList();
        range.end = params->GetSourceRange().end;
    }
    
    NamedDecl* namedDecl = nullptr;
    DeclContext* declContext = nullptr;

    if (!params) {
        RecordDecl* decl = sema.ActOnRecordDecl(identifier, range);
        if (!decl)
            return nullptr;
        namedDecl = decl;
        declContext = decl;
    } else {
        TemplateRecordDecl* decl = sema.ActOnTemplateRecordDecl(identifier, params, range);
        if (!decl)
            return nullptr;
        namedDecl = decl;
        declContext = decl;
    }
    
    ParserScopeGuard scopeGuard{ this, declContext };

    EXPECT_TOKEN(token, TokenKind::LeftBrace);
    NEXT_TOKEN();
    GET_TOKEN(token);
    while (token->kind != TokenKind::RightBrace) {
        if (!ParseRecordLevelDecl())
            return nullptr;
        GET_TOKEN(token);
    }
    NEXT_TOKEN();

    return namedDecl;
}

VCL::FieldDecl* VCL::Parser::ParseFieldDecl() {
    WithFullLoc<QualType> r = ParseQualType();
    if (r.value.GetAsOpaquePtr() == 0)
        return nullptr;
    Token* token;
    SourceRange range = r.range;
    EXPECT_TOKEN(token, TokenKind::Identifier);
    IdentifierInfo* identifier = token->identifier;
    range.end = token->range.end;
    NEXT_TOKEN();
    EXPECT_TOKEN(token, TokenKind::Semicolon);
    NEXT_TOKEN();
    return sema.ActOnFieldDecl(r.value, identifier, range);
}
#include <iostream>
VCL::NamedDecl* VCL::Parser::ParseAnyFunctionDecl() {
    WithFullLoc<QualType> returnType = ParseQualType();
    if (returnType.value.GetAsOpaquePtr() == 0)
        return nullptr;
    Token* token;
    SourceRange range = returnType.range;
    EXPECT_TOKEN(token, TokenKind::Identifier);
    IdentifierInfo* identifier = token->identifier;
    range.end = token->range.end;
    NEXT_TOKEN();
    GET_TOKEN(token);
    if (token->kind != TokenKind::Lesser) {
        return ParseEndFunctionDecl(returnType.value, identifier, range);
    } else {
        cc.GetDiagnosticReporter().Error(Diagnostic::MissingImplementation)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ range })
            .Report();
        return nullptr;
    }
}

VCL::FunctionDecl* VCL::Parser::ParseEndFunctionDecl(QualType type, IdentifierInfo* identifier, SourceRange range) {
    TransientFunctionDecl* transientFunctionDecl = sema.ActOnTransientFunctionDecl(type, identifier, range);
    llvm::SmallVector<QualType> paramsType{};
    if (!transientFunctionDecl)
        return nullptr;
    Token* token;
    {
        ParserScopeGuard sg{ this, transientFunctionDecl };
        EXPECT_TOKEN(token, TokenKind::LeftPar);
        NEXT_TOKEN();
        GET_TOKEN(token);
        if (token->kind != TokenKind::RightPar) {
            while (true) {
                ParamDecl* param = ParseParamDecl();
                if (!param)
                    return nullptr;
                paramsType.push_back(param->GetValueType());
                GET_TOKEN(token);
                if (token->kind == TokenKind::RightPar)
                    break;
                EXPECT_TOKEN(token, TokenKind::Coma);
                NEXT_TOKEN();
            }
        }
        EXPECT_TOKEN(token, TokenKind::RightPar);
        range.end = token->range.end;
    }
    NEXT_TOKEN();
    GET_TOKEN(token);
    FunctionDecl* functionDecl = sema.ActOnFunctionDecl(transientFunctionDecl, range);
    if (token->kind == TokenKind::Semicolon) {
        NEXT_TOKEN();
        return functionDecl;
    }
    ParserScopeGuard sg{ this, functionDecl };
    Stmt* body = ParseFunctionBody(functionDecl);
    if (!body)
        return nullptr;
    functionDecl->SetBody(body);
    return functionDecl;
}

VCL::NamedDecl* VCL::Parser::TryParseAnyFunctionDecl() {
    TentativeParsingGuard tp{ this };
    NamedDecl* decl = ParseAnyFunctionDecl();
    if (decl != nullptr)
        tp.Commit();
    return decl;
}

VCL::ParamDecl* VCL::Parser::ParseParamDecl() {
    Token* token;
    GET_TOKEN(token);
    SourceRange range = token->range;
    auto attr = ParseVarAttrBitfield();
    if (!attr.has_value())
        return nullptr;
    WithFullLoc<QualType> returnType = ParseQualType();
    if (returnType.value.GetAsOpaquePtr() == 0)
        return nullptr;
    EXPECT_TOKEN(token, TokenKind::Identifier);
    range.end = token->range.end;
    IdentifierInfo* identifier = token->identifier;
    NEXT_TOKEN();
    return sema.ActOnParamDecl(attr.value(), returnType.value, identifier, range);
}

VCL::CompoundStmt* VCL::Parser::ParseFunctionBody(FunctionDecl* function) {
    CompoundStmt* body = ParseCompoundStmt();
    if (!body)
        return nullptr;
    return sema.ActOnFunctionBody(function, body);
}

VCL::ReturnStmt* VCL::Parser::ParseReturnStmt() {
    Token* token;
    EXPECT_TOKEN(token, TokenKind::Keyword_return);
    SourceRange range = token->range;
    NEXT_TOKEN();
    GET_TOKEN(token);
    if (token->kind == TokenKind::Semicolon) {
        NEXT_TOKEN();
        return sema.ActOnReturnStmt(nullptr, range);
    }
    Expr* expr = ParseExpression();
    if (!expr)
        return nullptr;
    range.end = expr->GetSourceRange().end;
    EXPECT_TOKEN(token, TokenKind::Semicolon);
    NEXT_TOKEN();
    return sema.ActOnReturnStmt(expr, range);
}

VCL::VarDecl* VCL::Parser::ParseVarDecl() {
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
    return ParseEndVarDecl(attr, declType, identifier, declRange);
}

VCL::VarDecl* VCL::Parser::ParseEndVarDecl(VarDecl::VarAttrBitfield attr, QualType type, IdentifierInfo* identifier, SourceRange range) {
    Token* token;
    GET_TOKEN(token);
    Expr* initializer = nullptr;
    if (token->kind == TokenKind::Equal) {
        NEXT_TOKEN();
        initializer = ParseExpression();
        if (!initializer)
            return nullptr;
    }
    EXPECT_TOKEN(token, TokenKind::Semicolon);
    NEXT_TOKEN();
    VarDecl* decl = sema.ActOnVarDecl(type, identifier, attr, initializer, range);
    return decl;
}

VCL::VarDecl* VCL::Parser::TryParseVarDecl() {
    TentativeParsingGuard tp{ this };
    VarDecl* decl = ParseVarDecl();
    if (decl != nullptr)
        tp.Commit();
    return decl;
}

std::optional<VCL::VarDecl::VarAttrBitfield> VCL::Parser::ParseVarAttrBitfield() {
    VarDecl::VarAttrBitfield attr{ 0 };

    Token* token = GetToken();
    if (!token)
        return {};

    while (true) {
        TokenKind kind = token->kind;
        if (kind == TokenKind::Identifier)
            kind = token->identifier->GetTokenKind();
        switch (kind) {
            case TokenKind::Keyword_in:
                attr.hasInAttribute = 1;
                break;
            case TokenKind::Keyword_out:
                attr.hasOutAttribute = 1;
                break;
            case TokenKind::Keyword_inout:
                attr.hasInAttribute = 1;
                attr.hasOutAttribute = 1;
                break;
            default:
                return attr;
        }
        Token* token = NextAndGetToken();
        if (!token)
            return {};
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

VCL::TemplateParameterList* VCL::Parser::ParseTemplateParameterList() {
    llvm::SmallVector<NamedDecl*> params{};
    
    Token* token;
    SourceRange range;
    EXPECT_TOKEN(token, TokenKind::Lesser);
    range = token->range;
    NEXT_TOKEN();
    do {
        if (!params.empty()) {
            EXPECT_TOKEN(token, TokenKind::Coma);
            NEXT_TOKEN();
        }
        GET_TOKEN(token);
        TokenKind kind = token->kind;
        if (kind == TokenKind::Identifier)
            kind = token->identifier->GetTokenKind();
        switch (kind) {
            case TokenKind::Keyword_typename: {
                SourceRange range = token->range;
                NEXT_TOKEN();
                EXPECT_TOKEN(token, TokenKind::Identifier);
                IdentifierInfo* identifier = token->identifier;
                range.end = token->range.end;
                TemplateTypeParamDecl* decl = sema.ActOnTemplateTypeParamDecl(identifier, range);
                if (!decl)
                    return nullptr;
                params.emplace_back(decl);
                NEXT_TOKEN();
                break;
            }
            case TokenKind::Keyword_int8:
            case TokenKind::Keyword_int16:
            case TokenKind::Keyword_int32:
            case TokenKind::Keyword_int64:
            case TokenKind::Keyword_uint8:
            case TokenKind::Keyword_uint16:
            case TokenKind::Keyword_uint32:
            case TokenKind::Keyword_uint64: {
                WithFullLoc<Type*> r = ParseType();
                SourceRange range = r.range;
                if (r.value == nullptr)
                    return nullptr;
                if (r.value->GetTypeClass() != Type::BuiltinTypeClass) {
                    cc.GetDiagnosticReporter().Error(Diagnostic::WrongTemplateArgument)
                        .SetCompilerInfo(__FILE__, __func__, __LINE__)
                        .AddHint(DiagnosticHint{ SourceRange{ token->range.start, token->range.start } })
                        .Report();
                    return nullptr;
                }
                BuiltinType* type = (BuiltinType*)r.value;
                switch (type->GetKind()) {
                    case BuiltinType::Int8:
                    case BuiltinType::Int16:
                    case BuiltinType::Int32:
                    case BuiltinType::Int64:
                    case BuiltinType::UInt8:
                    case BuiltinType::UInt16:
                    case BuiltinType::UInt32:
                    case BuiltinType::UInt64:
                        break;
                    default: {
                        cc.GetDiagnosticReporter().Error(Diagnostic::WrongTemplateArgument)
                        .SetCompilerInfo(__FILE__, __func__, __LINE__)
                        .AddHint(DiagnosticHint{ SourceRange{ token->range.start, token->range.start } })
                        .Report();
                        return nullptr;
                    }
                }
                EXPECT_TOKEN(token, TokenKind::Identifier);
                IdentifierInfo* identifier = token->identifier;
                range.end = token->range.end;
                NonTypeTemplateParamDecl* decl = sema.ActOnNonTypeTemplateParamDecl(type, identifier, range);
                if (!decl)
                    return nullptr;
                params.emplace_back(decl);
                NEXT_TOKEN();
                break;
            }
            default: {
                cc.GetDiagnosticReporter().Error(Diagnostic::WrongTemplateArgument)
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .AddHint(DiagnosticHint{ SourceRange{ token->range.start, token->range.start } })
                    .Report();
                return nullptr;
            }
        }
        GET_TOKEN(token);
    } while (token->kind != TokenKind::Greater);
    range.end = token->range.end;
    NEXT_TOKEN();

    return sema.ActOnTemplateParameterList(params, range);
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
        if (Expr* expr = TryParseExpression(); expr != nullptr) {
            args.emplace_back(expr).SetSourceRange(expr->GetSourceRange());
        } else {
            WithFullLoc<QualType> type = ParseQualType();
            if (type.value.GetAsOpaquePtr() == 0)
                return nullptr;
            args.emplace_back(type.value).SetSourceRange(type.range);
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
        lhs = sema.ActOnBinaryExpr(lhs, rhs, op.kind);
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