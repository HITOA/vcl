#include <VCL/Sema/Template.hpp>

#include <VCL/Core/Diagnostic.hpp>
#include <VCL/AST/ExprEvaluator.hpp>
#include <VCL/Sema/Sema.hpp>

bool VCL::TemplateInstantiator::MakeTypeComplete(Type* type) {
    switch (type->GetTypeClass()) {
        case Type::TemplateSpecializationTypeClass: {
            TemplateSpecializationType* t = (TemplateSpecializationType*)type;
            if (t->GetInstantiatedType() != nullptr)
                return true;
            return InstantiateTemplateSpecializationType(t);
        }
        default:
            return true;
    }
}

bool VCL::TemplateInstantiator::InstantiateTemplateSpecializationType(TemplateSpecializationType* type) {
    if (!AddTemplateArgumentListAndDecl(type->GetTemplateArgumentList(), type->GetTemplateDecl()))
        return false;
    Type* t = nullptr;
    switch (type->GetTemplateDecl()->GetDeclClass()) {
        case Decl::IntrinsicTemplateDeclClass:
            t = InstantiateIntrinsicTemplateDecl((IntrinsicTemplateDecl*)type->GetTemplateDecl());
            break;
        case Decl::TemplateRecordDeclClass:
            t = InstantiateTemplateRecordDecl((TemplateRecordDecl*)type->GetTemplateDecl());
            break;
        default:
            sema.GetDiagnosticReporter().Error(Diagnostic::InternalError)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .AddHint(DiagnosticHint{ type->GetTemplateDecl()->GetSourceRange() })
                .Report();
            return false;
    }
    
    if (!t)
        return false;
    type->SetInstantiatedType(t);
    return true;
}

bool VCL::TemplateInstantiator::AddTemplateArgumentListAndDecl(TemplateArgumentList* args, TemplateDecl* decl) {
    if (!EvaluateTemplateArgumentsExpr(args) || !CheckTemplateArgumentsParametersMatch(args, decl->GetTemplateParametersList()))
        return false;
    for (size_t i = 0; i < decl->GetTemplateParametersList()->GetParams().size(); ++i)
        AddSubstitution(decl->GetTemplateParametersList()->GetParams()[i], &args->GetData()[i]);
    return true;
}

bool VCL::TemplateInstantiator::CheckTemplateArgumentsParametersMatch(TemplateArgumentList* args, TemplateParameterList* params) {
    if (args->GetArgs().size() < params->GetParams().size()) {
        sema.GetDiagnosticReporter().Error(Diagnostic::NotEnoughTemplateArgument)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ args->GetSourceRange() })
            .AddHint(DiagnosticHint{ params->GetSourceRange(), DiagnosticHint::Declared })
            .Report();
        return false;
    }
    if (args->GetArgs().size() > params->GetParams().size()) {
        sema.GetDiagnosticReporter().Error(Diagnostic::TooManyTemplateArgument)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ args->GetSourceRange() })
            .AddHint(DiagnosticHint{ params->GetSourceRange(), DiagnosticHint::Declared })
            .Report();
        return false;
    }
    for (size_t i = 0; i < args->GetArgs().size(); ++i) {
        const TemplateArgument& arg = args->GetArgs()[i];
        NamedDecl* param = params->GetParams()[i];
        switch (param->GetDeclClass()) {
            case Decl::TemplateTypeParamDeclClass: {
                if (arg.GetKind() != TemplateArgument::Type) {
                    if (param->GetSourceRange().start.GetPtr() != nullptr) {
                        sema.GetDiagnosticReporter().Error(Diagnostic::TemplateArgumentWrongType)
                            .SetCompilerInfo(__FILE__, __func__, __LINE__)
                            .AddHint(DiagnosticHint{ arg.GetSourceRange() })
                            .AddHint(DiagnosticHint{ param->GetSourceRange(), DiagnosticHint::Declared })
                            .Report();
                    } else {
                        sema.GetDiagnosticReporter().Error(Diagnostic::TemplateArgumentWrongType)
                            .SetCompilerInfo(__FILE__, __func__, __LINE__)
                            .AddHint(DiagnosticHint{ arg.GetSourceRange() })
                            .Report();
                    }
                    return false;
                }
                break;
            }
            case Decl::NonTypeTemplateParamDeclClass: {
                if (arg.GetKind() != TemplateArgument::Integral) {
                    if (param->GetSourceRange().start.GetPtr() != nullptr) {
                        sema.GetDiagnosticReporter().Error(Diagnostic::TemplateArgumentWrongType)
                            .SetCompilerInfo(__FILE__, __func__, __LINE__)
                            .AddHint(DiagnosticHint{ arg.GetSourceRange() })
                            .AddHint(DiagnosticHint{ param->GetSourceRange(), DiagnosticHint::Declared })
                            .Report();
                    } else {
                        sema.GetDiagnosticReporter().Error(Diagnostic::TemplateArgumentWrongType)
                            .SetCompilerInfo(__FILE__, __func__, __LINE__)
                            .AddHint(DiagnosticHint{ arg.GetSourceRange() })
                            .Report();
                    }
                    return false;
                }
                break;
            }
            default:
                sema.GetDiagnosticReporter().Error(Diagnostic::InternalError)
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .AddHint(DiagnosticHint{ args->GetSourceRange() })
                    .Report();
                return false;
        }
    }
    return true;
}

bool VCL::TemplateInstantiator::EvaluateTemplateArgumentsExpr(TemplateArgumentList* args) {
    ExprEvaluator eval{ sema.GetASTContext() };
    for (size_t i = 0; i < args->GetCount(); ++i) {
        TemplateArgument* arg = &args->GetData()[i];
        if (arg->GetKind() == TemplateArgument::Expression) {
            Expr* expr = arg->GetExpr();
            ConstantValue* value = eval.Visit(expr);
            if (!value) {
                sema.GetDiagnosticReporter().Error(Diagnostic::ExprDoesNotEvaluate)
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .AddHint(DiagnosticHint{ expr->GetSourceRange() })
                    .Report();
                return false;
            }
            if (value->GetConstantValueClass() != ConstantValue::ConstantScalarClass) {
                sema.GetDiagnosticReporter().Error(Diagnostic::ExprDoesNotEvaluateScalar)
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .AddHint(DiagnosticHint{ expr->GetSourceRange() })
                    .Report();
                return false;
            }
            ConstantScalar* scalar = (ConstantScalar*)value;
            SourceRange range = arg->GetSourceRange();
            *arg = TemplateArgument{ *scalar };
            arg->SetSourceRange(range);
        }
    }
    return true;
}

VCL::Type* VCL::TemplateInstantiator::InstantiateIntrinsicTemplateDecl(IntrinsicTemplateDecl* decl) {
    IdentifierInfo* identifier = decl->GetIdentifierInfo();
    switch (identifier->GetTokenKind()) {
        case TokenKind::Keyword_Vec: {
            TemplateArgument* arg0 = Lookup(decl->GetTemplateParametersList()->GetParams()[0]);
            QualType ofType = arg0->GetType();
            if (ofType.GetType()->GetTypeClass() != Type::BuiltinTypeClass) {
                sema.GetDiagnosticReporter().Error(Diagnostic::TemplateArgumentWrongType)
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .AddHint(DiagnosticHint{ arg0->GetSourceRange() })
                    .AddHint(DiagnosticHint{ decl->GetTemplateParametersList()->GetSourceRange(), DiagnosticHint::Declared })
                    .Report();
                return nullptr;
            }
            BuiltinType* type = (BuiltinType*)ofType.GetType();
            switch (type->GetKind()) {
                case BuiltinType::Bool:
                case BuiltinType::Float32:
                case BuiltinType::Float64:
                case BuiltinType::Int32:
                    break;
                default:
                    sema.GetDiagnosticReporter().Error(Diagnostic::TemplateArgumentWrongType)
                        .SetCompilerInfo(__FILE__, __func__, __LINE__)
                        .AddHint(DiagnosticHint{ arg0->GetSourceRange() })
                        .AddHint(DiagnosticHint{ decl->GetTemplateParametersList()->GetSourceRange(), DiagnosticHint::Declared })
                        .Report();
                    return nullptr;
            }
            return sema.GetASTContext().GetTypeCache().GetOrCreateVectorType(ofType);
        }
        case TokenKind::Keyword_Array: {
            TemplateArgument* arg0 = Lookup(decl->GetTemplateParametersList()->GetParams()[0]);
            TemplateArgument* arg1 = Lookup(decl->GetTemplateParametersList()->GetParams()[1]);
            QualType ofType = arg0->GetType();
            uint64_t ofSize = arg1->GetIntegral().Get<uint64_t>();
            return sema.GetASTContext().GetTypeCache().GetOrCreateArrayType(ofType, ofSize);
        }
        case TokenKind::Keyword_Span: {
            TemplateArgument* arg0 = Lookup(decl->GetTemplateParametersList()->GetParams()[0]);
            QualType ofType = arg0->GetType();
            return sema.GetASTContext().GetTypeCache().GetOrCreateSpanType(ofType);
        }
        default:
            sema.GetDiagnosticReporter().Error(Diagnostic::MissingImplementation)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
    }
}

VCL::RecordType* VCL::TemplateInstantiator::InstantiateTemplateRecordDecl(TemplateRecordDecl* decl) {
    RecordDecl* newDecl = RecordDecl::Create(sema.GetASTContext(), nullptr, decl->GetSourceRange());

    for (auto d = decl->Begin(); d != decl->End(); ++d) {
        switch (d->GetDeclClass()) {
            case Decl::TemplateTypeParamDeclClass:
            case Decl::NonTypeTemplateParamDeclClass:
                break;
            case Decl::FieldDeclClass: {
                FieldDecl* fieldDecl = (FieldDecl*)TransformDecl(d.Get());
                if (!MakeTypeComplete(fieldDecl->GetType().GetType()))
                    return nullptr;
                newDecl->InsertBack(fieldDecl);
                break;
            }
            default:
                sema.GetDiagnosticReporter().Error(Diagnostic::MissingImplementation)
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .AddHint(DiagnosticHint{ d->GetSourceRange() })
                    .Report();
                return nullptr;
        }
    }
    
    return (RecordType*)newDecl->GetType();
}

void VCL::TemplateInstantiator::AddSubstitution(NamedDecl* param, TemplateArgument* arg) {
    if (substitutionTable.count(param))
        return;
    substitutionTable[param] = arg;
}

VCL::TemplateArgument* VCL::TemplateInstantiator::Lookup(NamedDecl* param) {
    if (substitutionTable.count(param))
        return substitutionTable[param];
    return nullptr;
}

VCL::QualType VCL::TemplateInstantiator::TransformType(QualType type) {
    switch (type.GetType()->GetTypeClass()) {
        case Type::TemplateTypeParamTypeClass: return TransformTemplateTypeParamType(type);
        case Type::TemplateSpecializationTypeClass: return TransformTemplateSpecializationType(type);
        default: return type;
    }
}

VCL::Decl* VCL::TemplateInstantiator::TransformDecl(Decl* decl) {
    switch (decl->GetDeclClass()) {
        case Decl::FieldDeclClass: return TransformFieldDecl((FieldDecl*)decl);
        default: return decl;
    }
}

VCL::Expr* VCL::TemplateInstantiator::TransformExpr(Expr* expr) {
    switch (expr->GetExprClass())
    {
        case Expr::DeclRefExprClass: return TransformDeclRefExpr((DeclRefExpr*)expr);
        case Expr::CastExprClass: return TransformCastExpr((CastExpr*)expr);
        case Expr::BinaryExprClass: return TransformBinaryExpr((BinaryExpr*)expr);
        default: return expr;
    }
}

VCL::QualType VCL::TemplateInstantiator::TransformTemplateTypeParamType(QualType type) {
    TemplateTypeParamType* t = (TemplateTypeParamType*)type.GetType();
    TemplateArgument* arg = Lookup(t->GetTemplateTypeParamDecl());
    if (!arg) {
        sema.GetDiagnosticReporter().Error(Diagnostic::InternalError)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
        return QualType{};
    }
    if (arg->GetKind() != TemplateArgument::Type) {
        sema.GetDiagnosticReporter().Error(Diagnostic::WrongTemplateArgument)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ arg->GetSourceRange() })
            .Report();
        return nullptr;
    }
    QualType newType = arg->GetType();
    newType.AddQualifier(type.GetQualifiers());
    return newType;
}

VCL::QualType VCL::TemplateInstantiator::TransformTemplateSpecializationType(QualType type) {
    TemplateSpecializationType* t = (TemplateSpecializationType*)type.GetType();
    llvm::SmallVector<TemplateArgument> args{};
    for (auto& arg : t->GetTemplateArgumentList()->GetArgs()) {
        if (arg.GetKind() == TemplateArgument::Type) {
            args.emplace_back(TransformType(arg.GetType()));
        } else if (arg.GetKind() == TemplateArgument::Expression) {
            args.emplace_back(TransformExpr(arg.GetExpr()));
        } else {
            args.emplace_back(arg);
        }
    }
    TemplateArgumentList* arglist = TemplateArgumentList::Create(sema.GetASTContext(), args, t->GetTemplateArgumentList()->GetSourceRange());
    TemplateSpecializationType* newType = sema.GetASTContext().GetTypeCache().GetOrCreateTemplateSpecializationType(t->GetTemplateDecl(), arglist);
    return QualType{ newType, type.GetQualifiers() };
}

VCL::Decl* VCL::TemplateInstantiator::TransformFieldDecl(FieldDecl* decl) {
    QualType type = TransformType(decl->GetType());
    return FieldDecl::Create(sema.GetASTContext(), decl->GetIdentifierInfo(), type, decl->GetSourceRange());
}

VCL::Expr* VCL::TemplateInstantiator::TransformDeclRefExpr(DeclRefExpr* expr) {
    TemplateArgument* arg = Lookup(expr->GetValueDecl());
    if (!arg)
        return expr;
    switch (arg->GetKind()) {
        case TemplateArgument::Type: {
            sema.GetDiagnosticReporter().Error(Diagnostic::WrongTemplateArgument)
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .AddHint(DiagnosticHint{ expr->GetSourceRange() })
            .AddHint(DiagnosticHint{ arg->GetSourceRange(), DiagnosticHint::Declared })
            .Report();
            return nullptr;
        }
        case TemplateArgument::Expression: {
            return TransformExpr(arg->GetExpr());
        }
        case TemplateArgument::Integral: {
            return NumericLiteralExpr::Create(sema.GetASTContext(), arg->GetIntegral(), expr->GetSourceRange());
        }
        default:
            sema.GetDiagnosticReporter().Error(Diagnostic::MissingImplementation)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .AddHint(DiagnosticHint{ expr->GetSourceRange() })
                .Report();
            return nullptr;
    }
}

VCL::Expr* VCL::TemplateInstantiator::TransformCastExpr(CastExpr* expr) {
    Expr* arg = TransformExpr(expr->GetExpr());
    if (arg == expr->GetExpr())
        return expr;
    return CastExpr::Create(sema.GetASTContext(), arg, expr->GetCastKind(), expr->GetResultType(), expr->GetSourceRange());
}

VCL::Expr* VCL::TemplateInstantiator::TransformBinaryExpr(BinaryExpr* expr) {
    Expr* lhs = TransformExpr(expr->GetLHS());
    Expr* rhs = TransformExpr(expr->GetRHS());
    if (expr->GetLHS() == lhs && expr->GetRHS() == rhs)
        return expr;
    return BinaryExpr::Create(sema.GetASTContext(), lhs, rhs, expr->GetOperatorKind());
}