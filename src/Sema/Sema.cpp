#include <VCL/Sema/Sema.hpp>

#include <VCL/AST/ExprEvaluator.hpp>
#include <VCL/Sema/Template.hpp>

#include <llvm/ADT/SmallPtrSet.h>


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
    TemplateParameterList* vecParamList = TemplateParameterList::Create(astContext, llvm::ArrayRef<NamedDecl*>{ &vecOfType, 1 }, SourceRange{});
    IntrinsicTemplateDecl* vecDecl = IntrinsicTemplateDecl::Create(astContext, vecParamList, cc.GetIdentifierTable().GetKeyword(TokenKind::Keyword_Vec));
    astContext.GetTranslationUnitDecl()->InsertBack(vecDecl);

    // Array
    NamedDecl* arrayOfType = TemplateTypeParamDecl::Create(astContext, ofTypeIdentifier, SourceRange{});
    NamedDecl* arrayOfSize = NonTypeTemplateParamDecl::Create(astContext, ofSizeType, ofSizeIdentifier, SourceRange{});
    llvm::SmallVector<NamedDecl*> arrayParams{ arrayOfType, arrayOfSize };
    TemplateParameterList* arrayParamList = TemplateParameterList::Create(astContext, arrayParams, SourceRange{});
    IntrinsicTemplateDecl* arrayDecl = IntrinsicTemplateDecl::Create(astContext, arrayParamList, cc.GetIdentifierTable().GetKeyword(TokenKind::Keyword_Array));
    astContext.GetTranslationUnitDecl()->InsertBack(arrayDecl);

    // Span
    NamedDecl* spanOfType = TemplateTypeParamDecl::Create(astContext, ofTypeIdentifier, SourceRange{});
    TemplateParameterList* spanParamList = TemplateParameterList::Create(astContext, llvm::ArrayRef<NamedDecl*>{ &vecOfType, 1 }, SourceRange{});
    IntrinsicTemplateDecl* spanDecl = IntrinsicTemplateDecl::Create(astContext, vecParamList, cc.GetIdentifierTable().GetKeyword(TokenKind::Keyword_Span));
    astContext.GetTranslationUnitDecl()->InsertBack(spanDecl);

}

bool VCL::Sema::PushDeclContextScope(DeclContext* context) {
    sm.EmplaceScopeFront(context);
    return true;
}

bool VCL::Sema::PopDeclContextScope(DeclContext* context) {
    if (sm.GetScopeFront()->GetDeclContext() != context)  {
        cc.GetDiagnosticReporter().Error(Diagnostic::InternalError)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return false;
    }
    sm.PopScopeFront(context);
    return true;
}

VCL::RecordDecl* VCL::Sema::ActOnRecordDecl(IdentifierInfo* identifier, SourceRange range) {
    RecordDecl* decl = RecordDecl::Create(astContext, identifier, range);
    sm.GetScopeFront()->GetDeclContext()->InsertBack(decl);
    return decl;
}

VCL::TemplateRecordDecl* VCL::Sema::ActOnTemplateRecordDecl(IdentifierInfo* identifier, TemplateParameterList* params, SourceRange range) {
    TemplateRecordDecl* decl = TemplateRecordDecl::Create(astContext, identifier, params, range);
    sm.GetScopeFront()->GetDeclContext()->InsertBack(decl);
    return decl;
}

VCL::FieldDecl* VCL::Sema::ActOnFieldDecl(QualType type, IdentifierInfo* identifier, SourceRange range) {
    NamedDecl* decl = LookupNamedDecl(identifier, 1);
    if (decl) {
        SourceRange redeclRange = decl->GetSourceRange();
        cc.GetDiagnosticReporter().Error(Diagnostic::Redeclaration, identifier->GetName().str())
            .AddHint(DiagnosticHint{ range })
            .AddHint(DiagnosticHint{ redeclRange, DiagnosticHint::PreviouslyDeclared })
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return nullptr;
    }

    decl = FieldDecl::Create(astContext, identifier, type, range);
    sm.GetScopeFront()->GetDeclContext()->InsertBack(decl);
    return (FieldDecl*)decl;
}

VCL::VarDecl* VCL::Sema::ActOnVarDecl(QualType type, IdentifierInfo* identifier, VarDecl::VarAttrBitfield varAttrBitfield, Expr* initializer, SourceRange range) {
    if (identifier->IsKeyword()) {
        cc.GetDiagnosticReporter().Error(Diagnostic::ReservedIdentifier, identifier->GetName().str())
            .AddHint(DiagnosticHint{ range })
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return nullptr;
    }
    NamedDecl* decl = LookupNamedDecl(identifier, 1);
    if (decl) {
        SourceRange redeclRange = decl->GetSourceRange();
        cc.GetDiagnosticReporter().Error(Diagnostic::Redeclaration, identifier->GetName().str())
            .AddHint(DiagnosticHint{ range })
            .AddHint(DiagnosticHint{ redeclRange, DiagnosticHint::PreviouslyDeclared })
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return nullptr;
    }
    
    TemplateInstantiator instantiator{ *this };
    if (!instantiator.MakeTypeComplete(type.GetType()))
        return nullptr;

    decl = VarDecl::Create(astContext, type, identifier, varAttrBitfield, range);
    sm.GetScopeFront()->GetDeclContext()->InsertBack(decl);
    return (VarDecl*)decl;
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
        NamedDecl* decl = LookupNamedDecl(identifier);
        if (decl == nullptr) {
            cc.GetDiagnosticReporter().Error(Diagnostic::IdentifierUndefined, identifier->GetName().str())
                .AddHint(DiagnosticHint{ range })
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
        }
        if (list && !decl->IsTemplateDecl()) {
            cc.GetDiagnosticReporter().Error(Diagnostic::DoesNotTakeTemplateArgList, identifier->GetName().str())
                .AddHint(DiagnosticHint{ range })
                .AddHint(DiagnosticHint{ decl->GetSourceRange(), DiagnosticHint::Declared })
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
        }
        if (!list && decl->IsTemplateDecl()) {
            cc.GetDiagnosticReporter().Error(Diagnostic::MissingTemplateArgument, identifier->GetName().str())
                .AddHint(DiagnosticHint{ range })
                .AddHint(DiagnosticHint{ decl->GetSourceRange(), DiagnosticHint::Declared })
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
        }
        if (!list && !decl->IsTypeDecl()) {
            cc.GetDiagnosticReporter().Error(Diagnostic::NotTypeDecl, identifier->GetName().str())
                .AddHint(DiagnosticHint{ range })
                .AddHint(DiagnosticHint{ decl->GetSourceRange(), DiagnosticHint::Declared })
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
        }

        if (!list)
            return ((TypeDecl*)decl)->GetType();
        else
            return astContext.GetTypeCache().GetOrCreateTemplateSpecializationType((TemplateDecl*)decl, list);
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

VCL::TemplateParameterList* VCL::Sema::ActOnTemplateParameterList(llvm::ArrayRef<NamedDecl*> params, SourceRange range) {
    llvm::SmallPtrSet<IdentifierInfo*, 4> set{};
    
    for (auto param : params) {
        if (set.count(param->GetIdentifierInfo())) {
            NamedDecl* previous = nullptr;
            for (size_t i = 0; i < params.size(); ++i) {
                if (params[i]->GetIdentifierInfo() == param->GetIdentifierInfo()) {
                    previous = params[i];
                    break;
                }
            }
            cc.GetDiagnosticReporter().Error(Diagnostic::TemplateRedeclared, param->GetIdentifierInfo()->GetName().str())
                .AddHint(DiagnosticHint{ param->GetSourceRange() })
                .AddHint(DiagnosticHint{ previous->GetSourceRange(), DiagnosticHint::PreviouslyDeclared })
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
        }
        set.insert(param->GetIdentifierInfo());
    }

    return TemplateParameterList::Create(astContext, params, range);
}

VCL::TemplateArgumentList* VCL::Sema::ActOnTemplateArgumentList(llvm::ArrayRef<TemplateArgument> args, SourceRange range) {
    return TemplateArgumentList::Create(astContext, args, range);
}

VCL::TemplateTypeParamDecl* VCL::Sema::ActOnTemplateTypeParamDecl(IdentifierInfo* identifier, SourceRange range) {
    return TemplateTypeParamDecl::Create(astContext, identifier, range);
}

VCL::NonTypeTemplateParamDecl* VCL::Sema::ActOnNonTypeTemplateParamDecl(BuiltinType* type, IdentifierInfo* identifier, SourceRange range) {
    return NonTypeTemplateParamDecl::Create(astContext, type, identifier, range);
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
            cc.GetDiagnosticReporter().Error(Diagnostic::InternalError)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .AddHint(DiagnosticHint{ SourceRange{ lhs->GetSourceRange().start, rhs->GetSourceRange().end } })
                .Report();
            return nullptr;
    }
}

VCL::Expr* VCL::Sema::ActOnBinaryAssignmentExpr(Expr* lhs, Expr* rhs, BinaryOperator& op) {
    cc.GetDiagnosticReporter().Error(Diagnostic::InternalError)
        .SetCompilerInfo(__FILE__, __func__, __LINE__)
        .Report();
    return nullptr;
}

VCL::Expr* VCL::Sema::ActOnBinaryLogicalExpr(Expr* lhs, Expr* rhs, BinaryOperator& op) {
    cc.GetDiagnosticReporter().Error(Diagnostic::InternalError)
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
    NamedDecl* decl = LookupNamedDecl(identifier);
    if (decl == nullptr || !decl->IsValueDecl()) {
        cc.GetDiagnosticReporter().Error(Diagnostic::IdentifierUndefined, identifier->GetName().str())
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ range })
            .Report();
        return nullptr;
    }
    DeclRefExpr* expr = DeclRefExpr::Create(astContext, (ValueDecl*)decl, range);
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