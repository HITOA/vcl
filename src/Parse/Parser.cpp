#include <VCL/Parse/Parser.hpp>

#include <VCL/Core/Diagnostic.hpp>
#include <VCL/AST/Operator.hpp>
#include <VCL/AST/ExprEvaluator.hpp>
#include <VCL/Sema/Sema.hpp>

#define NEXT_TOKEN_N(n) if (!NextToken(n)) return nullptr
#define NEXT_TOKEN() if (!NextToken()) return nullptr
#define GET_TOKEN_N(token, n) token = GetToken(n); if (token == nullptr) return nullptr
#define GET_TOKEN(token) token = GetToken(); if (token == nullptr) return nullptr
#define EXPECT_TOKEN_N(token, kind, n) token = ExpectToken(kind, n, __FILE__, __func__, __LINE__); if (token == nullptr) return nullptr
#define EXPECT_TOKEN(token, kind) token = ExpectToken(kind, 0, __FILE__, __func__, __LINE__); if (token == nullptr) return nullptr


VCL::Parser::Parser(TokenStream& stream, Sema& sema) : stream{ stream }, sema{ sema } {

}


VCL::Token* VCL::Parser::ExpectToken(VCL::TokenKind kind, uint32_t n, const char* file, const char* func, uint32_t line) {
    Token* token = stream.GetTok(n);
    if (!token)
        return nullptr;
    TokenKind ckind = token->kind;
    if (token->kind == TokenKind::Identifier && kind != TokenKind::Identifier)
        ckind = token->identifier->GetTokenKind();
    if (ckind != kind) {
        sema.GetDiagnosticReporter().Error(Diagnostic::UnexpectedToken, std::string{ token->range.start.GetPtr(), token->range.end.GetPtr() })
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
        case TokenKind::Keyword_template:
            return ParseTemplateDecl();
        case TokenKind::Keyword_struct:
            return ParseRecordDecl();
        default: {
            int l = TryParseQualType();
            if (!l)
                return ParseVarDecl();
            kind = stream.GetTok(l + 1)->kind;
            if (kind == TokenKind::Equal || kind == TokenKind::Semicolon)
                return ParseVarDecl();
            return ParseFunctionDecl();
        }
    }
}

VCL::Decl* VCL::Parser::ParseRecordLevelDecl() {
    return ParseFieldDecl();
}

VCL::Stmt* VCL::Parser::ParseStmt(bool parseCompound) {
    Token* token;
    GET_TOKEN(token);
    TokenKind kind = token->kind;
    if (token->kind == TokenKind::Identifier)
        kind = token->identifier->GetTokenKind();
    switch (kind) {
        case TokenKind::Keyword_return: return ParseReturnStmt();
        case TokenKind::Keyword_if: return ParseIfStmt();
        case TokenKind::Keyword_while: return ParseWhileStmt();
        case TokenKind::Keyword_for: return ParseForStmt();
        case TokenKind::Keyword_break: return ParseBreakStmt();
        case TokenKind::Keyword_continue: return ParseContinueStmt();
        case TokenKind::LeftBrace: {
            if (!parseCompound) {
                sema.GetDiagnosticReporter().Error(Diagnostic::UnexpectedToken, std::string{ token->range.start.GetPtr(), token->range.end.GetPtr() })
                    .AddHint(DiagnosticHint{ token->range })
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .Report();
                return nullptr;
            }
            return ParseCompoundStmt();
        }
        case TokenKind::Keyword_in:
        case TokenKind::Keyword_out: {
            sema.GetDiagnosticReporter().Error(Diagnostic::AttrInvalidUse)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .AddHint(DiagnosticHint{ token->range })
                .Report();
            return nullptr;
        }
        default: {
            if (TryParseQualType()) {
                VarDecl* decl = ParseVarDecl();
                if (!decl)
                    return nullptr;
                return sema.ActOnDeclStmt(decl, decl->GetSourceRange());
            } else {
                Expr* expr = ParseExpression();
                if (!expr)
                    return nullptr;
                EXPECT_TOKEN(token, TokenKind::Semicolon);
                NEXT_TOKEN();
                return expr;
            }
        }
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
        Stmt* stmt = ParseStmt();
        if (!stmt)
            return nullptr;
        stmts.push_back(stmt);
        GET_TOKEN(token);
    }
    range.end = token->range.end;
    NEXT_TOKEN();

    return sema.ActOnCompoundStmt(stmts, range);
}

VCL::TemplateDecl* VCL::Parser::ParseTemplateDecl() {
    SourceRange range;
    Token* token;
    EXPECT_TOKEN(token, TokenKind::Keyword_template);
    range = token->range;
    NEXT_TOKEN();
    TemplateParameterList* parameters = ParseTemplateParameterList();
    if (!parameters)
        return nullptr;
    range.end = parameters->GetSourceRange().end;

    TemplateDecl* templateDecl = sema.ActOnTemplateDecl(parameters, range);

    ParserScopeGuard scopeGuard{ this, templateDecl };

    for (auto param : parameters->GetParams())
        if (!sema.AddDeclToScopeAndContext(param))
            return nullptr;

    GET_TOKEN(token);
    TokenKind kind = token->kind;
    if (token->kind == TokenKind::Identifier)
        kind = token->identifier->GetTokenKind();

    switch (kind) {
        case TokenKind::Keyword_struct: {
            NamedDecl* decl = ParseRecordDecl();
            if (!decl)
                return nullptr;
            templateDecl->SetTemplatedNamedDecl(decl);
            return templateDecl;
        }
        default: {
            NamedDecl* decl = ParseFunctionDecl();
            if (!decl)
                return nullptr;
            templateDecl->SetTemplatedNamedDecl(decl);
            return templateDecl;
        }
    }

    sema.GetDiagnosticReporter().Error(Diagnostic::UnexpectedToken, std::string{ token->range.start.GetPtr(), token->range.end.GetPtr() })
        .AddHint(DiagnosticHint{ token->range })
        .SetCompilerInfo(__FILE__, __func__, __LINE__)
        .Report();
    return nullptr;
}

VCL::NamedDecl* VCL::Parser::ParseRecordDecl() {
    SourceRange range;
    Token* token;
    EXPECT_TOKEN(token, TokenKind::Keyword_struct);
    range = token->range;
    NEXT_TOKEN();
    EXPECT_TOKEN(token, TokenKind::Identifier);
    IdentifierInfo* identifier = token->identifier;
    range.end = token->range.end;
    NEXT_TOKEN();
    
    RecordDecl* decl = sema.ActOnRecordDecl(identifier, range);

    ParserScopeGuard scopeGuard{ this, decl };

    EXPECT_TOKEN(token, TokenKind::LeftBrace);
    NEXT_TOKEN();
    GET_TOKEN(token);
    while (token->kind != TokenKind::RightBrace) {
        if (!ParseRecordLevelDecl())
            return nullptr;
        GET_TOKEN(token);
    }
    NEXT_TOKEN();

    return decl;
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

VCL::NamedDecl* VCL::Parser::ParseFunctionDecl() {
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
    if (token->kind == TokenKind::Lesser) {
        sema.GetDiagnosticReporter().Error(Diagnostic::MissingImplementation)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ range })
            .Report();
        return nullptr;
    }

    FunctionDecl* functionDecl = FunctionDecl::Create(sema.GetASTContext(), identifier);

    llvm::SmallVector<QualType> paramsType{};
    if (!functionDecl)
        return nullptr;

    {
        ParserScopeGuard sg{ this, functionDecl };
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
    functionDecl = sema.ActOnFunctionDecl(functionDecl, returnType.value, range);
    if (!functionDecl)
        return nullptr;
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

VCL::IfStmt* VCL::Parser::ParseIfStmt() {
    Token* token;
    EXPECT_TOKEN(token, TokenKind::Keyword_if);
    SourceRange range = token->range;
    NEXT_TOKEN();
    EXPECT_TOKEN(token, TokenKind::LeftPar);
    NEXT_TOKEN();

    ParserScopeGuard sg{ this, nullptr }; //If stmt scope guard

    Expr* condition = ParseExpression();
    if (!condition)
        return nullptr;

    EXPECT_TOKEN(token, TokenKind::RightPar);
    NEXT_TOKEN();

    Stmt* thenStmt = ParseStmt();
    if (!thenStmt)
        return nullptr;
    
    Stmt* elseStmt = nullptr;

    GET_TOKEN(token);
    TokenKind kind = token->kind;
    if (token->kind == TokenKind::Identifier)
        kind = token->identifier->GetTokenKind();
    if (kind == TokenKind::Keyword_else) {
        NEXT_TOKEN();
        elseStmt = ParseStmt();
        if (!elseStmt)
            return nullptr;
    }

    return sema.ActOnIfStmt(condition, thenStmt, elseStmt, range);
}

VCL::WhileStmt* VCL::Parser::ParseWhileStmt() {
    Token* token;
    EXPECT_TOKEN(token, TokenKind::Keyword_while);
    SourceRange range = token->range;
    NEXT_TOKEN();

    ParserScopeGuard sg{ this, nullptr, true };

    EXPECT_TOKEN(token, TokenKind::LeftPar);
    NEXT_TOKEN();
    Expr* condition = ParseExpression();
    if (!condition)
        return nullptr;
    EXPECT_TOKEN(token, TokenKind::RightPar);
    NEXT_TOKEN();
    Stmt* thenStmt = ParseStmt();
    if (!thenStmt)
        return nullptr;
    return sema.ActOnWhileStmt(condition, thenStmt, range);
}

VCL::ForStmt* VCL::Parser::ParseForStmt() {
    Token* token;
    EXPECT_TOKEN(token, TokenKind::Keyword_for);
    SourceRange range = token->range;
    NEXT_TOKEN();

    ParserScopeGuard sg{ this, nullptr, true };

    Stmt* startStmt = nullptr;
    Expr* condition = nullptr;
    Expr* loopExpr = nullptr;

    EXPECT_TOKEN(token, TokenKind::LeftPar);
    NEXT_TOKEN();
    GET_TOKEN(token);

    if (token->kind != TokenKind::Semicolon) {
        if (TryParseQualType()) {
            VarDecl* decl = ParseVarDecl();
            startStmt = sema.ActOnDeclStmt(decl, decl->GetSourceRange());
            if (!startStmt)
                return nullptr;
        } else {
            startStmt = ParseExpression();
            if (!startStmt)
                return nullptr;
            EXPECT_TOKEN(token, TokenKind::Semicolon);
            NEXT_TOKEN();
        }
    } else {
        NEXT_TOKEN();
    }

    if (token->kind != TokenKind::Semicolon) {
        condition = ParseExpression();
        if (!condition)
            return nullptr;
    }
    EXPECT_TOKEN(token, TokenKind::Semicolon);
    NEXT_TOKEN();

    if (token->kind != TokenKind::RightPar) {
        loopExpr = ParseExpression();
        if (!loopExpr)
            return nullptr;
    }
    EXPECT_TOKEN(token, TokenKind::RightPar);
    NEXT_TOKEN();

    Stmt* thenStmt = ParseStmt();
    if (!thenStmt)
        return nullptr;
    
    return sema.ActOnForStmt(startStmt, condition, loopExpr, thenStmt, range);
}

VCL::BreakStmt* VCL::Parser::ParseBreakStmt() {
    Token* token;
    EXPECT_TOKEN(token, TokenKind::Keyword_break);
    SourceRange range = token->range;
    NEXT_TOKEN();
    EXPECT_TOKEN(token, TokenKind::Semicolon);
    NEXT_TOKEN();
    return sema.ActOnBreakStmt(range);
}

VCL::ContinueStmt* VCL::Parser::ParseContinueStmt() {
    Token* token;
    EXPECT_TOKEN(token, TokenKind::Keyword_continue);
    SourceRange range = token->range;
    NEXT_TOKEN();
    EXPECT_TOKEN(token, TokenKind::Semicolon);
    NEXT_TOKEN();
    return sema.ActOnContinueStmt(range);
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
    return sema.ActOnVarDecl(declType, identifier, attr, initializer, declRange);
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

int VCL::Parser::TryParseQualType(int n) {
    TentativeParsingGuard tp{ this };
    for (int i = 0; i < n; ++i) 
        if (!NextToken()) 
            return 0;
    uint32_t beg = stream.GetTokIndex();
    
    WithFullLoc<QualType> r = ParseQualType();
    if (r.value.GetType() != nullptr)
        return stream.GetTokIndex() - beg;
    return 0;
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
                    sema.GetDiagnosticReporter().Error(Diagnostic::WrongTemplateArgument)
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
                        sema.GetDiagnosticReporter().Error(Diagnostic::WrongTemplateArgument)
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
                sema.GetDiagnosticReporter().Error(Diagnostic::WrongTemplateArgument)
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
        if (TryParseQualType()) {
            WithFullLoc<QualType> type = ParseQualType();
            if (type.value.GetAsOpaquePtr() == 0)
                return nullptr;
            args.emplace_back(type.value).SetSourceRange(type.range);
        } else {
            Expr* expr = ParseExpression();
            if (!expr)
                return nullptr;
            args.emplace_back(expr).SetSourceRange(expr->GetSourceRange());
        }
        GET_TOKEN(token);
    } while (token->kind != TokenKind::Greater);
    range.end = token->range.end;
    NEXT_TOKEN();

    return sema.ActOnTemplateArgumentList(args, range);
}

int VCL::Parser::TryParseTemplateArgumentList(int n) {
    TentativeParsingGuard tp{ this };
    for (int i = 0; i < n; ++i) 
        if (!NextToken()) 
            return 0;
    uint32_t beg = stream.GetTokIndex();
    TemplateArgumentList* list = ParseTemplateArgumentList();
    if (list != nullptr)
        return stream.GetTokIndex() - beg;
    return 0;
}

VCL::Expr* VCL::Parser::ParseExpression() {
    Expr* expr = ParsePrefixExpression();
    if (expr == nullptr)
        return nullptr;
    return ParseBinaryExpression(expr, 0);
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
    Token* token;
    GET_TOKEN(token);
    switch (token->kind) {
        case TokenKind::LeftPar: {
            if (TryParseQualType(1)) {
                SourceRange range = token->range;
                NEXT_TOKEN();
                WithFullLoc<Type*> type = ParseType();
                EXPECT_TOKEN(token, TokenKind::RightPar);
                range.end = token->range.end;
                NEXT_TOKEN();
                Expr* expr = ParsePrefixExpression();
                if (!expr)
                    return nullptr;
                range.end = expr->GetSourceRange().end;
                return sema.ActOnCast(expr, QualType{ type.value }, range);
            } else {
                return ParsePostfixExpression();
            }
        }
        case TokenKind::PlusPlus: {
            SourceRange range = token->range;
            NEXT_TOKEN();
            Expr* expr = ParsePrefixExpression();
            if (!expr)
                return nullptr;
            range.end = expr->GetSourceRange().end;
            return sema.ActOnUnaryExpr(expr, UnaryOperator::PrefixIncrement, range);
        }
        case TokenKind::MinusMinus: {
            SourceRange range = token->range;
            NEXT_TOKEN();
            Expr* expr = ParsePrefixExpression();
            if (!expr)
                return nullptr;
            range.end = expr->GetSourceRange().end;
            return sema.ActOnUnaryExpr(expr, UnaryOperator::PrefixDecrement, range);
        }
        case TokenKind::Plus: {
            SourceRange range = token->range;
            NEXT_TOKEN();
            Expr* expr = ParsePrefixExpression();
            if (!expr)
                return nullptr;
            range.end = expr->GetSourceRange().end;
            return sema.ActOnUnaryExpr(expr, UnaryOperator::Plus, range);
        }
        case TokenKind::Minus: {
            SourceRange range = token->range;
            NEXT_TOKEN();
            Expr* expr = ParsePrefixExpression();
            if (!expr)
                return nullptr;
            range.end = expr->GetSourceRange().end;
            return sema.ActOnUnaryExpr(expr, UnaryOperator::Minus, range);
        }
        case TokenKind::Exclaim: {
            SourceRange range = token->range;
            NEXT_TOKEN();
            Expr* expr = ParsePrefixExpression();
            if (!expr)
                return nullptr;
            range.end = expr->GetSourceRange().end;
            return sema.ActOnUnaryExpr(expr, UnaryOperator::LogicalNot, range);
        }
        case TokenKind::Tilde: {
            SourceRange range = token->range;
            NEXT_TOKEN();
            Expr* expr = ParsePrefixExpression();
            if (!expr)
                return nullptr;
            range.end = expr->GetSourceRange().end;
            return sema.ActOnUnaryExpr(expr, UnaryOperator::BitwiseNot, range);
        }
        default:
            return ParsePostfixExpression();
    }
}

VCL::Expr* VCL::Parser::ParsePostfixExpression() {
    Expr* expr = ParsePrimaryExpression();
    if (!expr)
        return nullptr;

    Token* token;
    while (true) {
        GET_TOKEN(token);
        switch (token->kind) {
            case TokenKind::Period: {
                NEXT_TOKEN();
                EXPECT_TOKEN(token, TokenKind::Identifier);
                IdentifierInfo* fieldIdentifier = token->identifier;
                SourceRange range{ expr->GetSourceRange().start, token->range.end };
                expr = sema.ActOnFieldAccessExpr(expr, fieldIdentifier, range);
                if (!expr)
                    return nullptr;
                NEXT_TOKEN();
                break;
            }
            case TokenKind::PlusPlus: {
                SourceRange range = expr->GetSourceRange();
                range.end = token->range.end;
                expr = sema.ActOnUnaryExpr(expr, UnaryOperator::PostfixIncrement, range);
                if (!expr)
                    return nullptr;
                NEXT_TOKEN();
                break;
            }
            case TokenKind::MinusMinus: {
                SourceRange range = expr->GetSourceRange();
                range.end = token->range.end;
                expr = sema.ActOnUnaryExpr(expr, UnaryOperator::PostfixDecrement, range);
                if (!expr)
                    return nullptr;
                NEXT_TOKEN();
                break;
            }
            case TokenKind::LeftSquare: {
                SourceRange range = expr->GetSourceRange();
                NEXT_TOKEN();
                Expr* index = ParseExpression();
                if (!index)
                    return nullptr;
                EXPECT_TOKEN(token, TokenKind::RightSquare);
                range.end = token->range.end;
                NEXT_TOKEN();
                expr = sema.ActOnSubscriptExpr(expr, index, range);
                break;
            }
            default:
                return expr;
        }
    }
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
            return ParseParentExpression();
        case TokenKind::LeftBrace:
            return ParseAggregateExpr();
        default:
            sema.GetDiagnosticReporter().Error(Diagnostic::UnexpectedToken, std::string{ token->range.start.GetPtr(), token->range.end.GetPtr() })
                .AddHint(DiagnosticHint{ token->range })
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
    }
}

VCL::Expr* VCL::Parser::ParseParentExpression() {
    Token* token;
    EXPECT_TOKEN(token, TokenKind::LeftPar);
    NEXT_TOKEN();
    Expr* expr = ParseExpression();
    EXPECT_TOKEN(token, TokenKind::RightPar);
    NEXT_TOKEN();
    return expr;
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
    token = GetToken(1);
    if (!token)
        return nullptr;
    if (TryParseTemplateArgumentList(1) != 0)
        return ParseCallExpr();
    if (token->kind == TokenKind::LeftPar)
        return ParseCallExpr();
    GET_TOKEN(token);
    IdentifierInfo* identifier = token->identifier;
    SourceRange range = token->range;
    NEXT_TOKEN();
    return sema.ActOnIdentifierExpr(identifier, range);
}

VCL::Expr* VCL::Parser::ParseCallExpr() {
    Token* token;
    EXPECT_TOKEN(token, TokenKind::Identifier);
    IdentifierInfo* identifier = token->identifier;
    SourceRange range = token->range;
    TemplateArgumentList* templateArguments = nullptr;
    NEXT_TOKEN();
    GET_TOKEN(token);
    if (token->kind == TokenKind::Lesser)
        templateArguments = ParseTemplateArgumentList();
    EXPECT_TOKEN(token, TokenKind::LeftPar);
    NEXT_TOKEN();
    GET_TOKEN(token);
    llvm::SmallVector<Expr*> args{};
    if (token->kind != TokenKind::RightPar) {
        while (true) {
            Expr* arg = ParseExpression();
            if (!arg)
                return nullptr;
            args.push_back(arg);
            GET_TOKEN(token);
            if (token->kind == TokenKind::RightPar)
                break;
            EXPECT_TOKEN(token, TokenKind::Coma);
            NEXT_TOKEN();
        }
    }
    range.end = token->range.end;
    NEXT_TOKEN();
    return sema.ActOnCallExpr(identifier, args, templateArguments, range);
}

VCL::Expr* VCL::Parser::ParseAggregateExpr() {
    Token* token;
    EXPECT_TOKEN(token, TokenKind::LeftBrace);
    SourceRange range = token->range;
    NEXT_TOKEN();
    GET_TOKEN(token);
    llvm::SmallVector<Expr*> elements{};
    while (token->kind != TokenKind::RightBrace) {
        Expr* expr = ParseExpression();
        if (!expr)
            return nullptr;
        elements.push_back(expr);
        GET_TOKEN(token);
        if (token->kind == TokenKind::RightBrace)
            break;
        EXPECT_TOKEN(token, TokenKind::Coma);
        NEXT_TOKEN();
    }
    range.end = token->range.end;
    NEXT_TOKEN();
    return sema.ActOnAggregateExpr(elements, range);
}

VCL::Parser::TentativeParsingGuard::TentativeParsingGuard(Parser* parser) : parser{ parser }, sp{ parser->stream } {
    isSupressing = !parser->sema.GetDiagnosticReporter().GetSupressAll();
    if (isSupressing)
        parser->sema.GetDiagnosticReporter().SetSupressAll(true);
}

VCL::Parser::TentativeParsingGuard::~TentativeParsingGuard() {
    if (isSupressing)
        parser->sema.GetDiagnosticReporter().SetSupressAll(false);
}

VCL::Parser::ParserScopeGuard::ParserScopeGuard(Parser* parser, DeclContext* context, bool loopScope) : parser{ parser }, context{ context } {
    parser->sema.PushDeclContextScope(context, loopScope);
}

VCL::Parser::ParserScopeGuard::~ParserScopeGuard() { parser->sema.PopDeclContextScope(context); }