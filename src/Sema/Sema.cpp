#include <VCL/Sema/Sema.hpp>

#include <VCL/Core/Diagnostic.hpp>
#include <VCL/Core/Identifier.hpp>
#include <VCL/AST/ExprEvaluator.hpp>
#include <VCL/Sema/Template.hpp>

#include <llvm/ADT/SmallPtrSet.h>


VCL::Sema::Sema(ASTContext& astContext, DiagnosticReporter& diagnosticReporter, IdentifierTable& identifierTable) 
        : astContext{ astContext }, diagnosticReporter{ diagnosticReporter }, identifierTable{ identifierTable } {
    sm.EmplaceScopeFront(astContext.GetTranslationUnitDecl());
    AddBuiltinIntrinsicTemplateDecl();
}

void VCL::Sema::AddBuiltinIntrinsicTemplateDecl() {

    IdentifierInfo* ofTypeIdentifier = identifierTable.Get("T");
    IdentifierInfo* ofSizeIdentifier = identifierTable.Get("Size");
    BuiltinType* ofSizeType = astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::UInt64);

    // Vec
    NamedDecl* vecOfType = TemplateTypeParamDecl::Create(astContext, ofTypeIdentifier, SourceRange{});
    TemplateParameterList* vecParamList = TemplateParameterList::Create(astContext, llvm::ArrayRef<NamedDecl*>{ &vecOfType, 1 }, SourceRange{});
    IntrinsicTemplateDecl* vecDecl = IntrinsicTemplateDecl::Create(astContext, vecParamList, identifierTable.GetKeyword(TokenKind::Keyword_Vec));
    astContext.GetTranslationUnitDecl()->InsertBack(vecDecl);

    // Array
    NamedDecl* arrayOfType = TemplateTypeParamDecl::Create(astContext, ofTypeIdentifier, SourceRange{});
    NamedDecl* arrayOfSize = NonTypeTemplateParamDecl::Create(astContext, ofSizeType, ofSizeIdentifier, SourceRange{});
    llvm::SmallVector<NamedDecl*> arrayParams{ arrayOfType, arrayOfSize };
    TemplateParameterList* arrayParamList = TemplateParameterList::Create(astContext, arrayParams, SourceRange{});
    IntrinsicTemplateDecl* arrayDecl = IntrinsicTemplateDecl::Create(astContext, arrayParamList, identifierTable.GetKeyword(TokenKind::Keyword_Array));
    astContext.GetTranslationUnitDecl()->InsertBack(arrayDecl);

    // Span
    NamedDecl* spanOfType = TemplateTypeParamDecl::Create(astContext, ofTypeIdentifier, SourceRange{});
    TemplateParameterList* spanParamList = TemplateParameterList::Create(astContext, llvm::ArrayRef<NamedDecl*>{ &vecOfType, 1 }, SourceRange{});
    IntrinsicTemplateDecl* spanDecl = IntrinsicTemplateDecl::Create(astContext, vecParamList, identifierTable.GetKeyword(TokenKind::Keyword_Span));
    astContext.GetTranslationUnitDecl()->InsertBack(spanDecl);

}

bool VCL::Sema::PushDeclContextScope(DeclContext* context) {
    sm.EmplaceScopeFront(context);
    return true;
}

bool VCL::Sema::PopDeclContextScope(DeclContext* context) {
    if (sm.GetScopeFront()->GetDeclContext() != context)  {
        diagnosticReporter.Error(Diagnostic::InternalError)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return false;
    }
    sm.PopScopeFront(context);
    return true;
}

VCL::CompoundStmt* VCL::Sema::ActOnCompoundStmt(llvm::ArrayRef<Stmt*> stmts, SourceRange range) {
    return CompoundStmt::Create(astContext, stmts, range);
}

VCL::DeclStmt* VCL::Sema::ActOnDeclStmt(Decl* decl, SourceRange range) {
    return DeclStmt::Create(astContext, decl, range);
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
        diagnosticReporter.Error(Diagnostic::Redeclaration, identifier->GetName().str())
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

VCL::TransientFunctionDecl* VCL::Sema::ActOnTransientFunctionDecl(QualType returnType, IdentifierInfo* identifier, SourceRange range) {
    TemplateInstantiator instantiator{ *this };
    if (!instantiator.MakeTypeComplete(returnType.GetType()))
        return nullptr;
    
    NamedDecl* decl = LookupNamedDecl(identifier, 1);
    if (decl) {
        SourceRange redeclRange = decl->GetSourceRange();
        diagnosticReporter.Error(Diagnostic::Redeclaration, identifier->GetName().str())
            .AddHint(DiagnosticHint{ range })
            .AddHint(DiagnosticHint{ redeclRange, DiagnosticHint::PreviouslyDeclared })
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return nullptr;
    }

    return TransientFunctionDecl::Create(astContext, returnType, identifier, range);
}

VCL::FunctionDecl* VCL::Sema::ActOnFunctionDecl(TransientFunctionDecl* decl, SourceRange range) {
    FunctionDecl* functionDecl = FunctionDecl::Create(astContext, decl, range);
    sm.GetScopeFront()->GetDeclContext()->InsertBack(functionDecl);
    return functionDecl;
}

VCL::ParamDecl* VCL::Sema::ActOnParamDecl(VarDecl::VarAttrBitfield attr, QualType type, IdentifierInfo* identifier, SourceRange range) {
    TemplateInstantiator instantiator{ *this };
    if (!instantiator.MakeTypeComplete(type.GetType()))
        return nullptr;
    
    bool isPassedByReference = attr.hasOutAttribute || (!attr.hasInAttribute && TypePreferByReference(type.GetType()));
    if (isPassedByReference) {
        Type* refType = astContext.GetTypeCache().GetOrCreateReferenceType(type);
        type = QualType{ refType, type.GetQualifiers() };
    }

    NamedDecl* decl = LookupNamedDecl(identifier, 1);
    if (decl) {
        SourceRange redeclRange = decl->GetSourceRange();
        diagnosticReporter.Error(Diagnostic::Redeclaration, identifier->GetName().str())
            .AddHint(DiagnosticHint{ range })
            .AddHint(DiagnosticHint{ redeclRange, DiagnosticHint::PreviouslyDeclared })
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return nullptr;
    }

    decl = ParamDecl::Create(astContext, type, identifier, range);
    sm.GetScopeFront()->GetDeclContext()->InsertBack(decl);
    return (ParamDecl*)decl;
}

VCL::CompoundStmt* VCL::Sema::ActOnFunctionBody(FunctionDecl* function, CompoundStmt* body) {
    if (!ActRecOnFunctionBodyReturnStmt(function, body->GetStmts()))
        return nullptr;
    return body;
}

bool VCL::Sema::ActRecOnFunctionBodyReturnStmt(FunctionDecl* function, llvm::ArrayRef<Stmt*> stmts) {
    for (size_t i = 0; i < stmts.size(); ++i) {
        Stmt* stmt = stmts[i];
        if (stmt->GetStmtClass() == Stmt::ReturnStmtClass) {
            if (i + 1 < stmts.size()) {
                Stmt* ignoredStmt = stmts[i + 1];
                diagnosticReporter.Warn(Diagnostic::StatementNeverReached)
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .AddHint(DiagnosticHint{ ignoredStmt->GetSourceRange() })
                    .Report();
            }
            return true;
        }
    }

    bool isFunctionVoid = false;
    Type* returnType = function->GetType()->GetReturnType().GetType();
    if (returnType->GetTypeClass() == Type::BuiltinTypeClass)
        isFunctionVoid = ((BuiltinType*)returnType)->GetKind() == BuiltinType::Void;

    if (!isFunctionVoid) {
        diagnosticReporter.Error(Diagnostic::MissingReturnStmt)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ function->GetSourceRange() })
            .Report();
    }

    return isFunctionVoid;
}

VCL::ReturnStmt* VCL::Sema::ActOnReturnStmt(Expr* expr, SourceRange range) {
    FunctionDecl* function = GetFrontmostFunctionDecl();
    if (!function) {
        diagnosticReporter.Error(Diagnostic::ReturnStmtOutsideOfBody)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ range })
            .Report();
        return nullptr;
    }

    bool isFunctionVoid = false;

    Type* returnType = function->GetType()->GetReturnType().GetType();
    if (returnType->GetTypeClass() == Type::BuiltinTypeClass)
        isFunctionVoid = ((BuiltinType*)returnType)->GetKind() == BuiltinType::Void;

    if (expr != nullptr && isFunctionVoid) {
        diagnosticReporter.Error(Diagnostic::InvalidReturnStmtExpr)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ range })
            .Report();
        return nullptr;
    } else if (expr == nullptr && !isFunctionVoid) {
        diagnosticReporter.Error(Diagnostic::InvalidReturnStmtNoExpr)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ range })
            .Report();
        return nullptr;
    }

    if (expr) {
        expr = ActOnCast(ActOnLoad(expr), function->GetType()->GetReturnType(), range);
        if (!expr)
            return nullptr;
    }
    
    return ReturnStmt::Create(astContext, expr, range);
}

VCL::VarDecl* VCL::Sema::ActOnVarDecl(QualType type, IdentifierInfo* identifier, VarDecl::VarAttrBitfield varAttrBitfield, Expr* initializer, SourceRange range) {
    if (identifier->IsKeyword()) {
        diagnosticReporter.Error(Diagnostic::ReservedIdentifier, identifier->GetName().str())
            .AddHint(DiagnosticHint{ range })
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return nullptr;
    }
    NamedDecl* decl = LookupNamedDecl(identifier, 1);
    if (decl) {
        SourceRange redeclRange = decl->GetSourceRange();
        diagnosticReporter.Error(Diagnostic::Redeclaration, identifier->GetName().str())
            .AddHint(DiagnosticHint{ range })
            .AddHint(DiagnosticHint{ redeclRange, DiagnosticHint::PreviouslyDeclared })
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return nullptr;
    }
    
    TemplateInstantiator instantiator{ *this };
    if (!instantiator.MakeTypeComplete(type.GetType()))
        return nullptr;

    if ((varAttrBitfield.hasInAttribute || varAttrBitfield.hasOutAttribute) && !IsCurrentScopeGlobal()) {
        diagnosticReporter.Error(Diagnostic::AttrInvalidUse)
            .AddHint(DiagnosticHint{ range })
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return nullptr;
        
    } 

    if (IsCurrentScopeGlobal() && initializer) {
        if (varAttrBitfield.hasInAttribute) {
            diagnosticReporter.Error(Diagnostic::InitializerInputVarDecl)
                .AddHint(DiagnosticHint{ range })
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
        }
        if (initializer->GetResultType().GetType() != type.GetType()) {
            Expr* castedInitializer = ActOnCast(initializer, type, initializer->GetSourceRange());
            if (!castedInitializer)
                return nullptr;
            initializer = castedInitializer;
        }
        ExprEvaluator exprEvaluator{ astContext };
        ConstantValue* value = exprEvaluator.Visit(initializer);
        if (!value) {
            diagnosticReporter.Error(Diagnostic::ExprDoesNotEvaluate)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .AddHint(DiagnosticHint{ initializer->GetSourceRange() })
                .Report();
            return nullptr;
        }
        initializer->SetConstantValue(value);
    } else if (initializer) {
        initializer = ActOnCast(ActOnLoad(initializer), type, initializer->GetSourceRange());
    }

    decl = VarDecl::Create(astContext, type, identifier, varAttrBitfield, range);
    ((VarDecl*)decl)->SetInitializer(initializer);
    sm.GetScopeFront()->GetDeclContext()->InsertBack(decl);
    return (VarDecl*)decl;
}

VCL::QualType VCL::Sema::ActOnQualType(Type* type, Qualifier qualifiers, SourceRange range) {
    return QualType(type, qualifiers);
}

#define ASSERT_BUILTIN_TYPE_NOT_TEMPLATED(list) if (list != nullptr) { \
                diagnosticReporter.Error(Diagnostic::BuiltinTypeIsNotTemplated, identifier->GetName().str()) \
                    .AddHint(DiagnosticHint{ range })\
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)\
                    .Report(); \
                return nullptr; }
                
VCL::Type* VCL::Sema::ActOnType(IdentifierInfo* identifier, TemplateArgumentList* list, SourceRange range) {
    if (!identifier->IsKeyword()) {
        NamedDecl* decl = LookupNamedDecl(identifier);
        if (decl == nullptr) {
            diagnosticReporter.Error(Diagnostic::IdentifierUndefined, identifier->GetName().str())
                .AddHint(DiagnosticHint{ range })
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
        }
        if (list && !decl->IsTemplateDecl()) {
            diagnosticReporter.Error(Diagnostic::DoesNotTakeTemplateArgList, identifier->GetName().str())
                .AddHint(DiagnosticHint{ range })
                .AddHint(DiagnosticHint{ decl->GetSourceRange(), DiagnosticHint::Declared })
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
        }
        if (!list && decl->IsTemplateDecl()) {
            diagnosticReporter.Error(Diagnostic::MissingTemplateArgument, identifier->GetName().str())
                .AddHint(DiagnosticHint{ range })
                .AddHint(DiagnosticHint{ decl->GetSourceRange(), DiagnosticHint::Declared })
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
        }
        if (!list && !decl->IsTypeDecl()) {
            diagnosticReporter.Error(Diagnostic::NotTypeDecl, identifier->GetName().str())
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
                diagnosticReporter.Error(Diagnostic::ReservedIdentifier, identifier->GetName().str())
                    .AddHint(DiagnosticHint{ range })
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .Report();
                return nullptr;
            }
            if (!list && decl->IsTemplateDecl()) {
                diagnosticReporter.Error(Diagnostic::MissingTemplateArgument, identifier->GetName().str())
                    .AddHint(DiagnosticHint{ range })
                    .AddHint(DiagnosticHint{ decl->GetSourceRange(), DiagnosticHint::Declared })
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
            diagnosticReporter.Error(Diagnostic::TemplateRedeclared, param->GetIdentifierInfo()->GetName().str())
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
    llvm::SmallVector<TemplateArgument> argsEval{};

    for (auto arg : args) {
        switch (arg.GetKind()) {
            case TemplateArgument::Expression: {
                ExprEvaluator eval{ astContext };
                ConstantValue* value = eval.Visit(arg.GetExpr());
                if (!value) {
                    argsEval.push_back(arg);
                    break;
                } else if (value->GetConstantValueClass() != ConstantValue::ConstantScalarClass) {
                    argsEval.push_back(arg);
                    break;
                }
                argsEval.push_back(TemplateArgument{ *(ConstantScalar*)value });
                break;
            }
            default:
                argsEval.push_back(arg);
        }
    }

    return TemplateArgumentList::Create(astContext, argsEval, range);
}

VCL::TemplateTypeParamDecl* VCL::Sema::ActOnTemplateTypeParamDecl(IdentifierInfo* identifier, SourceRange range) {
    return TemplateTypeParamDecl::Create(astContext, identifier, range);
}

VCL::NonTypeTemplateParamDecl* VCL::Sema::ActOnNonTypeTemplateParamDecl(BuiltinType* type, IdentifierInfo* identifier, SourceRange range) {
    return NonTypeTemplateParamDecl::Create(astContext, type, identifier, range);
}

VCL::Expr* VCL::Sema::ActOnBinaryExpr(Expr* lhs, Expr* rhs, BinaryOperator::Kind op) {
    switch (op) {
        // Arithmetic
        case BinaryOperator::Add:
        case BinaryOperator::Sub:
        case BinaryOperator::Mul:
        case BinaryOperator::Div:
        case BinaryOperator::Remainder: {
            lhs = ActOnLoad(lhs);
            rhs = ActOnLoad(rhs);
            if (lhs->GetResultType() != rhs->GetResultType()) {
                // Try implicite cast
                std::pair<Expr*, Expr*> r = ActOnImplicitBinaryArithmeticCast(lhs, rhs);
                lhs = r.first;
                rhs = r.second;
                if (!lhs || !rhs)
                    return nullptr;
            }
            Expr* expr = BinaryExpr::Create(astContext, lhs, rhs, op);
            expr->SetValueCategory(Expr::RValue);
            return expr;
        }
        //Logical
        case BinaryOperator::Greater:
        case BinaryOperator::Lesser:
        case BinaryOperator::GreaterEqual:
        case BinaryOperator::LesserEqual:
        case BinaryOperator::Equal:
        case BinaryOperator::NotEqual: {
            lhs = ActOnLoad(lhs);
            rhs = ActOnLoad(rhs);
            if (lhs->GetResultType() != rhs->GetResultType()) {
                // Try implicite cast
                std::pair<Expr*, Expr*> r = ActOnImplicitBinaryArithmeticCast(lhs, rhs);
                lhs = r.first;
                rhs = r.second;
                if (!lhs || !rhs)
                    return nullptr;
            }
            Expr* expr = BinaryExpr::Create(astContext, lhs, rhs, op);
            expr->SetValueCategory(Expr::RValue);
            expr->SetResultType(astContext.GetTypeCache().GetOrCreateBuiltinType(BuiltinType::Bool));
            return expr;
        }
        case BinaryOperator::LogicalAnd:
        case BinaryOperator::LogicalOr: {
            diagnosticReporter.Error(Diagnostic::MissingImplementation)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
        }
        // Assignment
        case BinaryOperator::Assignment: {
            rhs = ActOnLoad(rhs);
            if (!IsExprAssignable(lhs))
                return nullptr;
            rhs = ActOnCast(rhs, lhs->GetResultType(), SourceRange{ lhs->GetSourceRange().start, rhs->GetSourceRange().end });
            if (!rhs) return nullptr;
            Expr* expr = BinaryExpr::Create(astContext, lhs, rhs, op);
            expr->SetValueCategory(Expr::LValue);
            return expr;
        }
        case BinaryOperator::AssignmentAdd:
            rhs = ActOnBinaryExpr(lhs, rhs, BinaryOperator::Add);
            if (!rhs) return nullptr;
            return ActOnBinaryExpr(lhs, rhs, BinaryOperator::Assignment);
        case BinaryOperator::AssignmentSub:
            rhs = ActOnBinaryExpr(lhs, rhs, BinaryOperator::Sub);
            if (!rhs) return nullptr;
            return ActOnBinaryExpr(lhs, rhs, BinaryOperator::Assignment);
        case BinaryOperator::AssignmentMul:
            rhs = ActOnBinaryExpr(lhs, rhs, BinaryOperator::Mul);
            if (!rhs) return nullptr;
            return ActOnBinaryExpr(lhs, rhs, BinaryOperator::Assignment);
        case BinaryOperator::AssignmentDiv:
            rhs = ActOnBinaryExpr(lhs, rhs, BinaryOperator::Div);
            if (!rhs) return nullptr;
            return ActOnBinaryExpr(lhs, rhs, BinaryOperator::Assignment);
        default: {
            diagnosticReporter.Error(Diagnostic::MissingImplementation)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
        }
    }
}

bool VCL::Sema::IsExprAssignable(Expr* expr) {
    if (expr->GetValueCategory() != Expr::LValue) {
        diagnosticReporter.Error(Diagnostic::AssignmentNotLValue)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ expr->GetSourceRange() })
            .Report();
        return false;
    }

    if (expr->GetResultType().HasQualifier(Qualifier::Const)) {
        diagnosticReporter.Error(Diagnostic::AssignmentConstValue)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ expr->GetSourceRange() })
            .Report();
        return false;
    }

    if (expr->GetExprClass() == Expr::DeclRefExprClass) {
        ValueDecl* decl = ((DeclRefExpr*)expr)->GetValueDecl();
        switch (decl->GetDeclClass()) {
            case Decl::VarDeclClass:
                if (((VarDecl*)decl)->HasInAttribute()) {
                    diagnosticReporter.Error(Diagnostic::AssignmentInputValue)
                        .SetCompilerInfo(__FILE__, __func__, __LINE__)
                        .AddHint(DiagnosticHint{ expr->GetSourceRange() })
                        .Report();
                    return false;
                }
                return true;
            default:
                return true;
        }
    }
    return true;
}

VCL::Expr* VCL::Sema::ActOnFieldAccessExpr(Expr* lhs, IdentifierInfo* field, SourceRange range) {
    if (lhs->GetValueCategory() != Expr::LValue) {
        diagnosticReporter.Error(Diagnostic::MustBeLValue)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ range })
            .Report();
        return nullptr;
    }

    Type* type = lhs->GetResultType().GetType();
    if (type->GetTypeClass() == Type::ReferenceTypeClass)
        type = ((ReferenceType*)type)->GetType().GetType();
    if (type->GetTypeClass() == Type::TemplateSpecializationTypeClass)
        type = ((TemplateSpecializationType*)type)->GetInstantiatedType();
    if (type->GetTypeClass() != Type::RecordTypeClass) {
        diagnosticReporter.Error(Diagnostic::MustHaveStructType)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ range })
            .Report();
        return nullptr;
    }

    RecordDecl* recordDecl = ((RecordType*)type)->GetRecordDecl();

    FieldDecl* correctFieldDecl = nullptr;
    uint32_t fieldIdx = 0;

    for (auto it = recordDecl->Begin(); it != recordDecl->End(); ++it) {
        if (it->GetDeclClass() != Decl::FieldDeclClass)
            continue;
        FieldDecl* fieldDecl = (FieldDecl*)it.Get();
        if (fieldDecl->GetIdentifierInfo() == field) {
            correctFieldDecl = fieldDecl;
            break;
        }
        ++fieldIdx;
    }

    if (!correctFieldDecl) {
        diagnosticReporter.Error(Diagnostic::MissingMember, recordDecl->GetIdentifierInfo()->GetName().str(), field->GetName().str())
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ range })
            .AddHint(DiagnosticHint{ recordDecl->GetSourceRange(), DiagnosticHint::Declared })
            .Report();
        return nullptr;
    }

    QualType returnedFieldType = correctFieldDecl->GetType();
    if (lhs->GetResultType().HasQualifier(Qualifier::Const))
        returnedFieldType.AddQualifier(Qualifier::Const);

    return FieldAccessExpr::Create(astContext, lhs, (RecordType*)type, fieldIdx, returnedFieldType, range);
}

VCL::Expr* VCL::Sema::ActOnLoad(Expr* expr) {
    if (expr->GetValueCategory() != Expr::LValue)
        return expr;
    return LoadExpr::Create(astContext, expr, expr->GetSourceRange());
}

std::pair<VCL::Expr*, VCL::Expr*> VCL::Sema::ActOnImplicitBinaryArithmeticCast(Expr* lhs, Expr* rhs) {
    // Todo
    if (diagnosticReporter.Warn(Diagnostic::MissingImplementation).SetCompilerInfo(__FILE__, __func__, __LINE__).Report())
        return std::make_pair(nullptr, nullptr);
    return std::make_pair(lhs, ActOnCast(rhs, lhs->GetResultType(), SourceRange{ lhs->GetSourceRange().start, rhs->GetSourceRange().end }));
}

VCL::Expr* VCL::Sema::ActOnCast(Expr* expr, QualType toType, SourceRange range) {
    Type* srcType = GetInstantiatedType(expr->GetResultType().GetType());
    Type* dstType = GetInstantiatedType(toType.GetType());

    if (!srcType || !dstType) {
        diagnosticReporter.Error(Diagnostic::InternalError)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return nullptr;
    }

    BuiltinType::Kind srcKind;
    BuiltinType::Kind dstKind;

    while (dstType->GetTypeClass() == Type::ReferenceTypeClass)
        dstType = ((ReferenceType*)dstType)->GetType().GetType();
    
    if (srcType == dstType)
        return expr;

    if (!CheckTypeCastability(srcType) || !CheckTypeCastability(dstType)) {
        diagnosticReporter.Error(Diagnostic::InvalidCast)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ expr->GetSourceRange() })
            .Report();
        return nullptr;
    }

    if (srcType->GetTypeClass() == Type::VectorTypeClass && dstType->GetTypeClass() != Type::VectorTypeClass) {
        diagnosticReporter.Error(Diagnostic::InvalidVectorCast)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ expr->GetSourceRange() })
            .Report();
        return nullptr;
    }

    if (dstType->GetTypeClass() == Type::VectorTypeClass && srcType->GetTypeClass() == Type::BuiltinTypeClass) {
        expr = ActOnSplat(expr, expr->GetSourceRange());
        if (!expr)
            return nullptr;
    }
    
    srcKind = GetScalarKindFromBuiltinOrVectorType(srcType);
    dstKind = GetScalarKindFromBuiltinOrVectorType(dstType);

    uint32_t srcBitWidth = BuiltinType::GetKindBitWidth(srcKind);
    BuiltinType::Category srcCategory = BuiltinType::GetKindCategory(srcKind);

    uint32_t dstBitWidth = BuiltinType::GetKindBitWidth(dstKind);
    BuiltinType::Category dstCategory = BuiltinType::GetKindCategory(dstKind);

    CastExpr::CastKind kind;

    if (srcCategory == dstCategory) {
        uint32_t o = srcCategory == BuiltinType::FloatingPointKind ? CastExpr::FloatingCastExt : CastExpr::SignedCastExt;
        if (srcCategory == BuiltinType::UnsignedKind)
            o = CastExpr::UnsignedCastExt;

        if (srcBitWidth < dstBitWidth) {
            kind = (CastExpr::CastKind)o;
        } else {
            kind = (CastExpr::CastKind)(o + 1);
        }
    } else {
        if (srcCategory == BuiltinType::FloatingPointKind) {
            if (dstCategory == BuiltinType::SignedKind)
                kind = CastExpr::FloatingToSigned;
            else
                kind = CastExpr::FloatingToUnsigned;
        } else if (srcCategory == BuiltinType::SignedKind) {
            if (dstCategory == BuiltinType::FloatingPointKind)
                kind = CastExpr::SignedToFloating;
            else
                kind = CastExpr::SignedToUnsigned;
        } else if (srcCategory == BuiltinType::UnsignedKind) {
            if (dstCategory == BuiltinType::FloatingPointKind)
                kind = CastExpr::UnsignedToFloating;
            else
                kind = CastExpr::UnsignedToSigned;
        }
    }

    Expr* castExpr = CastExpr::Create(astContext, ActOnLoad(expr), kind, toType, range);
    return castExpr;
}

VCL::Expr* VCL::Sema::ActOnSplat(Expr* expr, SourceRange range) {
    Type* type = expr->GetResultType().GetType();
    if (type->GetTypeClass() != Type::BuiltinTypeClass) {
        diagnosticReporter.Error(Diagnostic::InvalidSplat)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ expr->GetSourceRange() })
            .Report();
        return nullptr;
    }
    BuiltinType* builtinType = (BuiltinType*)type;
    if (builtinType->GetKind() == BuiltinType::Void) {
        diagnosticReporter.Error(Diagnostic::InvalidSplat)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ expr->GetSourceRange() })
            .Report();
        return nullptr;
    }
    return SplatExpr::Create(astContext, ActOnLoad(expr), range);
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
        diagnosticReporter.Error(Diagnostic::IdentifierUndefined, identifier->GetName().str())
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ range })
            .Report();
        return nullptr;
    }
    DeclRefExpr* expr = DeclRefExpr::Create(astContext, (ValueDecl*)decl, range);
    expr->SetValueCategory(Expr::LValue);
    return expr;
}

VCL::Expr* VCL::Sema::ActOnCallExpr(IdentifierInfo* identifier, llvm::ArrayRef<Expr*> args, SourceRange range) {
    FunctionDecl* decl = LookupFunctionDecl(identifier);
    if (!decl) {
        diagnosticReporter.Error(Diagnostic::IdentifierUndefined, identifier->GetName().str())
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ range })
            .Report();
        return nullptr;
    }

    FunctionType* type = decl->GetType();
    if (type->GetParamsType().size() < args.size()) {
        diagnosticReporter.Error(Diagnostic::TooManyArgument)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ range })
            .Report();
        return nullptr;
    }

    llvm::SmallVector<Expr*> trueArgs{};
    for (size_t i = 0; i < args.size(); ++i) {
        Expr* arg = args[i];
        QualType paramType = type->GetParamsType()[i];
        if (paramType.GetType()->GetTypeClass() == Type::ReferenceTypeClass) {
            QualType actualType = ((ReferenceType*)paramType.GetType())->GetType();
            if (arg->GetValueCategory() != Expr::LValue) {
                diagnosticReporter.Error(Diagnostic::MustBeLValue)
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .AddHint(DiagnosticHint{ arg->GetSourceRange() })
                    .Report();
                return nullptr;
            }
            if (arg->GetResultType().GetType() != actualType.GetType()) {
                diagnosticReporter.Error(Diagnostic::IncorrectType)
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .AddHint(DiagnosticHint{ arg->GetSourceRange() })
                    .Report();
                return nullptr;
            }
            if ((arg->GetResultType().GetQualifiers() & paramType.GetQualifiers()) != arg->GetResultType().GetQualifiers()) {
                diagnosticReporter.Error(Diagnostic::QualifierDropped)
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .AddHint(DiagnosticHint{ arg->GetSourceRange() })
                    .Report();
                return nullptr;
            }
            trueArgs.push_back(arg);
        } else {
            Expr* castedArg = ActOnCast(ActOnLoad(arg), paramType, arg->GetSourceRange());
            if (!castedArg)
                return nullptr;
            trueArgs.push_back(castedArg);
        }
    }

    if (trueArgs.size() < type->GetParamsType().size()) {
        diagnosticReporter.Error(Diagnostic::MissingArgument)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ range })
            .Report();
        return nullptr;
    }

    return CallExpr::Create(astContext, decl, trueArgs, type->GetReturnType(), range);
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

VCL::FunctionDecl* VCL::Sema::LookupFunctionDecl(IdentifierInfo* identifier, int depth) {
    Scope* currentScope = sm.GetScopeFront();

    int currentDepth = 0;
    while (currentScope != nullptr && (currentDepth < depth || depth == -1)) {
        DeclContext* declContext = currentScope->GetDeclContext();
        for (auto it = declContext->Begin(); it != declContext->End(); ++it) {
            if (it->GetDeclClass() != Decl::FunctionDeclClass)
                continue;
            FunctionDecl* decl = (FunctionDecl*)it.Get();
            if (identifier == decl->GetIdentifierInfo())
                return decl;
        }
        currentScope = currentScope->GetParentScope();
        ++currentDepth;
    }

    return nullptr;
}

VCL::FunctionDecl* VCL::Sema::GetFrontmostFunctionDecl() {
    Scope* currentScope = sm.GetScopeFront();

    while (currentScope != nullptr) {
        DeclContext* declContext = currentScope->GetDeclContext();
        
        if (declContext->GetDeclContextClass() == DeclContext::FunctionDeclContextClass)
            return (FunctionDecl*)declContext;

        currentScope = currentScope->GetParentScope();
    }

    return nullptr;
}

bool VCL::Sema::TypePreferByReference(Type* type) {
    type = GetInstantiatedType(type);
    if (type->GetTypeClass() == Type::BuiltinTypeClass || type->GetTypeClass() == Type::VectorTypeClass)
        return false;
    return true;
}

VCL::Type* VCL::Sema::GetInstantiatedType(Type* type) {
    if (type->GetTypeClass() != Type::TemplateSpecializationTypeClass)
        return type;
    return ((TemplateSpecializationType*)type)->GetInstantiatedType();
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

VCL::BuiltinType::Kind VCL::Sema::GetScalarKindFromBuiltinOrVectorType(Type* type) {
    if (type->GetTypeClass() == Type::BuiltinTypeClass)
        return ((BuiltinType*)type)->GetKind();
    VectorType* vectorType = (VectorType*)type;
    return ((BuiltinType*)vectorType->GetElementType().GetType())->GetKind();
}

bool VCL::Sema::IsCurrentScopeGlobal() {
    DeclContext* frontDeclContext = sm.GetScopeFront()->GetDeclContext();
    return frontDeclContext->GetDeclContextClass() == DeclContext::TranslationUnitDeclContextClass;
}