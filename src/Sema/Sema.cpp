#include <VCL/Sema/Sema.hpp>

#include <VCL/AST/ExprEvaluator.hpp>


VCL::Sema::Sema(ASTContext& astContext, CompilerContext& cc) : astContext{ astContext }, cc{ cc } {
    sm.EmplaceScopeFront(astContext.GetTranslationUnitDecl());
    AddBuiltinIntrinsicTemplateDecl();
}

void VCL::Sema::AddBuiltinIntrinsicTemplateDecl() {

    IdentifierInfo* ofTypeIdentifier = cc.GetIdentifierTable().Get("T");
    IdentifierInfo* ofSizeIdentifier = cc.GetIdentifierTable().Get("Size");
    BuiltinType* ofSizeType = astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::UInt64);

    // Vec
    NamedDecl* vecOfType = TemplateTypeParamDecl::Create(astContext, ofTypeIdentifier, SourceRange{});
    TemplateParameterList* vecParamList = TemplateParameterList::Create(astContext, llvm::ArrayRef<NamedDecl*>{ &vecOfType, 1 });
    IntrinsicTemplateDecl* vecDecl = IntrinsicTemplateDecl::Create(astContext, vecParamList, cc.GetIdentifierTable().GetKeyword(TokenKind::Keyword_Vec));
    astContext.GetTranslationUnitDecl()->InsertBack(vecDecl);

    // Array
    NamedDecl* arrayOfType = TemplateTypeParamDecl::Create(astContext, ofTypeIdentifier, SourceRange{});
    NamedDecl* arrayOfSize = NonTypeTemplateParamDecl::Create(astContext, ofSizeType, ofSizeIdentifier, SourceRange{});
    llvm::SmallVector<NamedDecl*> arrayParams{ arrayOfType, arrayOfSize };
    TemplateParameterList* arrayParamList = TemplateParameterList::Create(astContext, arrayParams);
    IntrinsicTemplateDecl* arrayDecl = IntrinsicTemplateDecl::Create(astContext, arrayParamList, cc.GetIdentifierTable().GetKeyword(TokenKind::Keyword_Array));
    astContext.GetTranslationUnitDecl()->InsertBack(arrayDecl);

    // Span
    NamedDecl* spanOfType = TemplateTypeParamDecl::Create(astContext, ofTypeIdentifier, SourceRange{});
    TemplateParameterList* spanParamList = TemplateParameterList::Create(astContext, llvm::ArrayRef<NamedDecl*>{ &vecOfType, 1 });
    IntrinsicTemplateDecl* spanDecl = IntrinsicTemplateDecl::Create(astContext, vecParamList, cc.GetIdentifierTable().GetKeyword(TokenKind::Keyword_Span));
    astContext.GetTranslationUnitDecl()->InsertBack(spanDecl);

}

VCL::VarDecl* VCL::Sema::ActOnVarDecl(QualType type, IdentifierInfo* identifier, VarDecl::VarAttrBitfield varAttrBitfield, Expr* initializer, SourceRange range) {
    VarDecl* decl = LookupVarDecl(identifier, 1);
    if (decl) {
        SourceRange redeclRange = decl->GetSourceRange();
        cc.GetDiagnosticReporter().Error(Diagnostic::Redeclaration, identifier->GetName().str())
            .AddHint(DiagnosticHint{ range })
            .AddHint(DiagnosticHint{ redeclRange, DiagnosticHint::PreviouslyDeclared })
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return nullptr;
    }

    decl = VarDecl::Create(astContext, type, identifier, varAttrBitfield, range);
    sm.GetScopeFront()->GetDeclContext()->InsertBack(decl);
    return decl;
}

VCL::QualType VCL::Sema::ActOnQualType(Type* type, Qualifier qualifiers, SourceRange range) {
    return QualType(type, qualifiers);
}

#define ASSERT_BUILTIN_TYPE_NOT_TEMPLATED(list) if (list != nullptr) { \
                cc.GetDiagnosticReporter().Error(Diagnostic::BuiltinTypeIsNotTemplated, identifier->GetName().str()) \
                    .AddHint(DiagnosticHint{ range })\
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)\
                    .Report(); \
                return nullptr; }
                
VCL::Type* VCL::Sema::ActOnType(IdentifierInfo* identifier, TemplateArgumentList* list, SourceRange range) {
    if (!identifier->IsKeyword()) {
        TypeDecl* decl = LookupTypeDecl(identifier);
        if (decl == nullptr) {
            cc.GetDiagnosticReporter().Error(Diagnostic::IdentifierUndefined, identifier->GetName().str())
                .AddHint(DiagnosticHint{ range })
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
        }
        cc.GetDiagnosticReporter().Error(Diagnostic::MissingImplementation)
            .AddHint(DiagnosticHint{ range })
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return nullptr;
    }

    switch (identifier->GetTokenKind()) {
        case TokenKind::Keyword_void:
            ASSERT_BUILTIN_TYPE_NOT_TEMPLATED(list);
            return astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::Void);
        case TokenKind::Keyword_bool:
            ASSERT_BUILTIN_TYPE_NOT_TEMPLATED(list);
            return astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::Bool);
        case TokenKind::Keyword_uint8:
            ASSERT_BUILTIN_TYPE_NOT_TEMPLATED(list);
            return astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::UInt8);
        case TokenKind::Keyword_uint16:
            ASSERT_BUILTIN_TYPE_NOT_TEMPLATED(list);
            return astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::UInt16);
        case TokenKind::Keyword_uint32:
            ASSERT_BUILTIN_TYPE_NOT_TEMPLATED(list);
            return astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::UInt32);
        case TokenKind::Keyword_uint64:
            ASSERT_BUILTIN_TYPE_NOT_TEMPLATED(list);
            return astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::UInt64);
        case TokenKind::Keyword_int8:
            ASSERT_BUILTIN_TYPE_NOT_TEMPLATED(list);
            return astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::Int8);
        case TokenKind::Keyword_int16:
            ASSERT_BUILTIN_TYPE_NOT_TEMPLATED(list);
            return astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::Int16);
        case TokenKind::Keyword_int32:
            ASSERT_BUILTIN_TYPE_NOT_TEMPLATED(list);
            return astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::Int32);
        case TokenKind::Keyword_int64:
            ASSERT_BUILTIN_TYPE_NOT_TEMPLATED(list);
            return astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::Int64);
        case TokenKind::Keyword_float32:
            ASSERT_BUILTIN_TYPE_NOT_TEMPLATED(list);
            return astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::Float32);
        case TokenKind::Keyword_float64:
            ASSERT_BUILTIN_TYPE_NOT_TEMPLATED(list);
            return astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::Float64);
        default:
            IntrinsicTemplateDecl* decl = LookupIntrinsicTemplateDecl(identifier);
            if (decl == nullptr) {
                cc.GetDiagnosticReporter().Error(Diagnostic::MissingImplementation)
                    .AddHint(DiagnosticHint{ range })
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .Report();
                return nullptr;
            }
            return astContext.GetTypeCache().GetOrCreateTemplateSpecializationType(decl, list);
    }
}

VCL::TemplateArgumentList* VCL::Sema::ActOnTemplateArgumentList(llvm::ArrayRef<TemplateArgument> args, SourceRange range) {
    return TemplateArgumentList::Create(astContext, args, range);
}

VCL::Expr* VCL::Sema::ActOnBinaryExpr(Expr* lhs, Expr* rhs, BinaryOperator& op) {
    switch (op.kind) {
        // Binary Arithmetic
        case BinaryOperator::Mul:
        case BinaryOperator::Div:
        case BinaryOperator::Remainder:
        case BinaryOperator::Add:
        case BinaryOperator::Sub:
            return ActOnBinaryArithmeticExpr(lhs, rhs, op);
        // Binary Logical
        case BinaryOperator::Greater:
        case BinaryOperator::Lesser:
        case BinaryOperator::GreaterEqual:
        case BinaryOperator::LesserEqual:
        case BinaryOperator::Equal:
        case BinaryOperator::NotEqual:
        case BinaryOperator::LogicalAnd:
        case BinaryOperator::LogicalOr:
            return ActOnBinaryLogicalExpr(lhs, rhs, op);
        default:
            cc.GetDiagnosticReporter().Error(Diagnostic::InternalSemaError)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .AddHint(DiagnosticHint{ SourceRange{ lhs->GetSourceRange().start, rhs->GetSourceRange().end } })
                .Report();
            return nullptr;
    }
}

VCL::Expr* VCL::Sema::ActOnBinaryAssignmentExpr(Expr* lhs, Expr* rhs, BinaryOperator& op) {
    cc.GetDiagnosticReporter().Error(Diagnostic::InternalSemaError)
        .SetCompilerInfo(__FILE__, __func__, __LINE__)
        .Report();
    return nullptr;
}

VCL::Expr* VCL::Sema::ActOnBinaryLogicalExpr(Expr* lhs, Expr* rhs, BinaryOperator& op) {
    cc.GetDiagnosticReporter().Error(Diagnostic::InternalSemaError)
        .SetCompilerInfo(__FILE__, __func__, __LINE__)
        .Report();
    return nullptr;
}

VCL::Expr* VCL::Sema::ActOnBinaryArithmeticExpr(Expr* lhs, Expr* rhs, BinaryOperator& op) {
    // Check for expr type
    if (lhs->GetResultType() != rhs->GetResultType()) {
        // Try implicite cast
        std::pair<Expr*, Expr*> r = ActOnImplicitBinaryArithmeticCast(lhs, rhs);
        lhs = r.first;
        rhs = r.second;
        if (!lhs || !rhs)
            return nullptr;
    }

    Expr* expr = BinaryArithmeticExpr::Create(astContext, lhs, rhs, op);
    return expr;
}


std::pair<VCL::Expr*, VCL::Expr*> VCL::Sema::ActOnImplicitBinaryArithmeticCast(Expr* lhs, Expr* rhs) {
    // Todo
    if (cc.GetDiagnosticReporter().Warn(Diagnostic::MissingImplementation).SetCompilerInfo(__FILE__, __func__, __LINE__).Report())
        return std::make_pair(nullptr, nullptr);
    return std::make_pair(lhs, ActOnCast(rhs, lhs->GetResultType(), SourceRange{ lhs->GetSourceRange().start, rhs->GetSourceRange().end }));
}

VCL::Expr* VCL::Sema::ActOnCast(Expr* expr, QualType toType, SourceRange range) {
    Type* srcType = expr->GetResultType().GetType();
    Type* dstType = toType.GetType();

    if (!CheckTypeCastability(srcType) || !CheckTypeCastability(dstType)) {
        cc.GetDiagnosticReporter().Error(Diagnostic::InvalidCast)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ expr->GetSourceRange() })
            .Report();
        return nullptr;
    }

    Expr* castExpr = CastExpr::Create(astContext, expr, toType, range);
    return castExpr;
}

VCL::Expr* VCL::Sema::ActOnNumericConstant(Token* value) {
    llvm::StringRef valueStr{ value->range.start.GetPtr(), (size_t)(value->range.end.GetPtr() - value->range.start.GetPtr()) };
    if (value->isFloatingPoint) {
        double v = std::stod(valueStr.str());
        return NumericLiteralExpr::Create(astContext, ConstantScalar{ v }, value->range);
    } else {
        uint64_t v = std::stoull(valueStr.str());
        return NumericLiteralExpr::Create(astContext, ConstantScalar{ v }, value->range);
    }
}

VCL::Expr* VCL::Sema::ActOnIdentifierExpr(IdentifierInfo* identifier, SourceRange range) {
    VarDecl* decl = LookupVarDecl(identifier);
    if (decl == nullptr) {
        cc.GetDiagnosticReporter().Error(Diagnostic::IdentifierUndefined, identifier->GetName().str())
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ range })
            .Report();
        return nullptr;
    }
    VariableRefExpr* expr = VariableRefExpr::Create(astContext, decl, range);
    return expr;
}

VCL::NamedDecl* VCL::Sema::LookupNamedDecl(IdentifierInfo* identifier, int depth) {
    Scope* currentScope = sm.GetScopeFront();

    int currentDepth = 0;
    while (currentScope != nullptr && (currentDepth < depth || depth == -1)) {
        DeclContext* declContext = currentScope->GetDeclContext();
        for (auto it = declContext->Begin(); it != declContext->End(); ++it) {
            if (!it->IsNamedDecl())
                continue;
            NamedDecl* decl = (NamedDecl*)it.Get();
            if (identifier == decl->GetIdentifierInfo())
                return decl;
        }
        currentScope = currentScope->GetParentScope();
        ++currentDepth;
    }
    
    return nullptr;
}

VCL::TypeDecl* VCL::Sema::LookupTypeDecl(IdentifierInfo* identifier, int depth) {
    Scope* currentScope = sm.GetScopeFront();

    int currentDepth = 0;
    while (currentScope != nullptr && (currentDepth < depth || depth == -1)) {
        DeclContext* declContext = currentScope->GetDeclContext();
        for (auto it = declContext->Begin(); it != declContext->End(); ++it) {
            if (!it->IsTypeDecl())
                continue;
            TypeDecl* decl = (TypeDecl*)it.Get();
            if (identifier == decl->GetIdentifierInfo())
                return decl;
        }
        currentScope = currentScope->GetParentScope();
        ++currentDepth;
    }

    return nullptr;
}

VCL::IntrinsicTemplateDecl* VCL::Sema::LookupIntrinsicTemplateDecl(IdentifierInfo* identifier, int depth) {
    Scope* currentScope = sm.GetScopeFront();

    int currentDepth = 0;
    while (currentScope != nullptr && (currentDepth < depth || depth == -1)) {
        DeclContext* declContext = currentScope->GetDeclContext();
        for (auto it = declContext->Begin(); it != declContext->End(); ++it) {
            if (!it->GetDeclClass() == Decl::IntrinsicTemplateDeclClass)
                continue;
            IntrinsicTemplateDecl* decl = (IntrinsicTemplateDecl*)it.Get();
            if (identifier == decl->GetIdentifierInfo())
                return decl;
        }
        currentScope = currentScope->GetParentScope();
        ++currentDepth;
    }

    return nullptr;
}

VCL::VarDecl* VCL::Sema::LookupVarDecl(IdentifierInfo* identifier, int depth) {
    Scope* currentScope = sm.GetScopeFront();

    int currentDepth = 0;
    while (currentScope != nullptr && (currentDepth < depth || depth == -1)) {
        DeclContext* declContext = currentScope->GetDeclContext();
        for (auto it = declContext->Begin(); it != declContext->End(); ++it) {
            if (it->GetDeclClass() != Decl::VarDeclClass)
                continue;
            VarDecl* decl = (VarDecl*)it.Get();
            if (identifier == decl->GetIdentifierInfo())
                return decl;
        }
        currentScope = currentScope->GetParentScope();
        ++currentDepth;
    }

    return nullptr;
}

bool VCL::Sema::CheckTypeCastability(Type* type) {
    switch (type->GetTypeClass()) {
        case Type::BuiltinTypeClass:
            return ((BuiltinType*)type)->GetKind() != BuiltinType::Void;
        case Type::VectorTypeClass:
            return true;
        default:
            return false;
    }
}