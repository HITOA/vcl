#include <VCL/Sema/Template.hpp>

#include <VCL/Core/Diagnostic.hpp>
#include <VCL/AST/ExprEvaluator.hpp>
#include <VCL/Sema/Sema.hpp>
#include <VCL/AST/TypePrinter.hpp>



bool VCL::TemplateInstantiator::MakeTypeComplete(Type* type) {
    switch (type->GetTypeClass()) {
        case Type::ReferenceTypeClass:
            return MakeTypeComplete(((ReferenceType*)type)->GetType().GetType());
        case Type::TemplateSpecializationTypeClass: {
            TemplateSpecializationType* t = (TemplateSpecializationType*)type;
            if (t->GetInstantiatedType() != nullptr)
                return true;
            return InstantiateTemplateSpecializationType(t);
        }
        case Type::TypeAliasTypeClass:
            return MakeTypeComplete(((TypeAliasType*)type)->GetType());
        default:
            return true;
    }
}

bool VCL::TemplateInstantiator::InstantiateTemplateSpecializationType(TemplateSpecializationType* type) {
    Sema::SemaContextGuard guard{ sema, sema.GetInstantiationContext(type->GetTemplateDecl(), type->GetTemplateArgumentList()) };
    if (!type->GetTemplateArgumentList()->IsCanonical()) {
        TemplateArgumentList* args = sema.ActOnTemplateArgumentList(
            type->GetTemplateArgumentList()->GetArgs(), 
            type->GetTemplateArgumentList()->GetSourceRange(), 
            true);
        if (!args)
            return false;
        TemplateSpecializationType* ct = sema.GetASTContext().GetTypeCache().GetOrCreateTemplateSpecializationType(
            type->GetTemplateDecl(), args);
        
        llvm::FoldingSetNodeID id{};
        TemplateSpecializationType::Profile(id, type->GetTemplateDecl(), args);

        if (ct->GetInstantiatedType() == nullptr)
            if (!InstantiateTemplateSpecializationType(ct))
                return false;
        type->SetInstantiatedType(ct->GetInstantiatedType());
        return true;
    }
    if (!AddTemplateArgumentListAndDecl(type->GetTemplateArgumentList(), type->GetTemplateDecl()))
        return false;
    Type* t = nullptr;
    switch (type->GetTemplateDecl()->GetTemplatedNamedDecl()->GetDeclClass()) {
        case Decl::IntrinsicTypeDeclClass:
            t = InstantiateTemplatedIntrinsicTypeDecl(type->GetTemplateDecl());
            break;
        case Decl::RecordDeclClass:
            t = InstantiateTemplatedRecordDecl(type->GetTemplateDecl());
            break;
        case Decl::TypeAliasDeclClass:
            t = InstantiateTemplatedTypeAliasDecl(type->GetTemplateDecl());
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

VCL::Type* VCL::TemplateInstantiator::InstantiateTemplatedIntrinsicTypeDecl(TemplateDecl* decl) {
    IntrinsicTypeDecl* intrinsicDecl = (IntrinsicTypeDecl*)decl->GetTemplatedNamedDecl();
    IdentifierInfo* identifier = intrinsicDecl->GetIdentifierInfo();
    switch (identifier->GetTokenKind()) {
        case TokenKind::Keyword_Vec: {
            TemplateArgument* arg0 = Lookup(decl->GetTemplateParametersList()->GetParams()[0]);
            QualType ofType = Type::GetCanonicalType(arg0->GetType().GetType());
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
        case TokenKind::Keyword_Lanes: {
            TemplateArgument* arg0 = Lookup(decl->GetTemplateParametersList()->GetParams()[0]);
            QualType ofType = Type::GetCanonicalType(arg0->GetType().GetType());
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
            return sema.GetASTContext().GetTypeCache().GetOrCreateLanesType(ofType);
        }
        case TokenKind::Keyword_Array: {
            TemplateArgument* arg0 = Lookup(decl->GetTemplateParametersList()->GetParams()[0]);
            TemplateArgument* arg1 = Lookup(decl->GetTemplateParametersList()->GetParams()[1]);
            QualType ofType = arg0->GetType();
            uint64_t ofSize = arg1->GetIntegral().Get<uint64_t>();
            if (!MakeTypeComplete(ofType.GetType()))
                return nullptr;
            return sema.GetASTContext().GetTypeCache().GetOrCreateArrayType(ofType, ofSize);
        }
        case TokenKind::Keyword_Span: {
            TemplateArgument* arg0 = Lookup(decl->GetTemplateParametersList()->GetParams()[0]);
            QualType ofType = arg0->GetType();
            if (!MakeTypeComplete(ofType.GetType()))
                return nullptr;
            return sema.GetASTContext().GetTypeCache().GetOrCreateSpanType(ofType);
        }
        default:
            sema.GetDiagnosticReporter().Error(Diagnostic::MissingImplementation)
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
            return nullptr;
    }
}

VCL::Type* VCL::TemplateInstantiator::InstantiateTemplatedRecordDecl(TemplateDecl* decl) {
    RecordDecl* recordDecl = (RecordDecl*)decl->GetTemplatedNamedDecl();
    RecordDecl* newDecl = RecordDecl::Create(sema.GetASTContext(), recordDecl->GetIdentifierInfo(), recordDecl->GetSourceRange());

    for (auto d = recordDecl->Begin(); d != recordDecl->End(); ++d) {
        switch (d->GetDeclClass()) {
            case Decl::FieldDeclClass: {
                FieldDecl* fieldDecl = (FieldDecl*)TransformDecl(d.Get());
                if (!fieldDecl)
                    return nullptr;
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

VCL::Type* VCL::TemplateInstantiator::InstantiateTemplatedTypeAliasDecl(TemplateDecl* decl) {
    TypeAliasDecl* typeAliasDecl = (TypeAliasDecl*)decl->GetTemplatedNamedDecl();
    QualType concreteType = TransformType(typeAliasDecl->GetType());
    return sema.GetASTContext().GetTypeCache().GetOrCreateTypeAliasType(concreteType.GetType(), typeAliasDecl);
}

VCL::FunctionDecl* VCL::TemplateInstantiator::InstantiateTemplatedFunctionDecl(TemplateDecl* decl) {
    // Allocated in the caller's current ASTContext: see Sema::GetInstantiationContext.
    FunctionDecl* functionDecl = (FunctionDecl*)decl->GetTemplatedNamedDecl();

    FunctionDecl* newFunctionDecl = FunctionDecl::Create(sema.GetASTContext(), functionDecl->GetIdentifierInfo());
    if (!newFunctionDecl)
        return nullptr;

    if (functionDecl->HasFunctionFlag(FunctionDecl::IsIntrinsic)) {
        newFunctionDecl->SetIntrinsicID(functionDecl->GetIntrinsicID());
        newFunctionDecl->SetFunctionFlag(FunctionDecl::IsIntrinsic);
    }
    
    QualType returnType = TransformType(functionDecl->GetType()->GetReturnType());

    if (!returnType.GetAsOpaquePtr())
        return nullptr;
 
    sema.PushDeclContextScope(newFunctionDecl);
    
    for (auto it = functionDecl->Begin(); it != functionDecl->End(); ++it) {
        if (it->GetDeclClass() != Decl::ParamDeclClass)
            continue;
        
        ParamDecl* paramDecl = (ParamDecl*)it.Get();
        ParamDecl* newParamDecl = (ParamDecl*)TransformDecl(paramDecl);

        if (!newParamDecl) {
            sema.PopDeclContextScope(newFunctionDecl);
            return nullptr;
        }
    }

    if (FunctionDecl* r = sema.ActOnFunctionDecl(newFunctionDecl, returnType, nullptr, functionDecl->GetSourceRange()); r != nullptr) {
        newFunctionDecl = r;
    } else {
        sema.PopDeclContextScope(newFunctionDecl);
        return nullptr;
    }

    if (!functionDecl->HasFunctionFlag(FunctionDecl::IsIntrinsic)) {
        Stmt* body = TransformFunctionBody(functionDecl);
        if (!body) {
            sema.PopDeclContextScope(newFunctionDecl);
            return nullptr;
        }

        newFunctionDecl->SetBody(body);
    }

    sema.PopDeclContextScope(newFunctionDecl);

    newFunctionDecl->SetFunctionFlag(FunctionDecl::IsTemplateSpecialization);

    return newFunctionDecl;
}

void VCL::TemplateInstantiator::AddSubstitution(NamedDecl* param, TemplateArgument* arg) {
    substitutionTable[param] = arg;
}

VCL::TemplateArgument* VCL::TemplateInstantiator::Lookup(NamedDecl* param) {
    if (substitutionTable.count(param))
        return substitutionTable[param];
    return nullptr;
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
    TemplateArgumentList* arglist = TransformTemplateArgumentList(t->GetTemplateArgumentList());
    if (!t || !arglist)
        return nullptr;
    TemplateSpecializationType* newType = sema.GetASTContext().GetTypeCache().GetOrCreateTemplateSpecializationType(t->GetTemplateDecl(), arglist);
    if (!MakeTypeComplete(newType))
        return nullptr;
    return QualType{ newType, type.GetQualifiers() };
}

VCL::QualType VCL::TemplateInstantiator::TransformTypeAliasType(QualType type) {
    return TransformType(((TypeAliasType*)type.GetType())->GetType());
}

VCL::Expr* VCL::TemplateInstantiator::TransformDeclRefExpr(DeclRefExpr* expr) {
    TemplateArgument* arg = Lookup(expr->GetValueDecl());
    if (!arg)
        return TreeTransform::TransformDeclRefExpr(expr);
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
