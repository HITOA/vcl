#include <VCL/Sema/Template.hpp>

#include <VCL/Core/Diagnostic.hpp>
#include <VCL/AST/ExprEvaluator.hpp>
#include <VCL/Sema/Sema.hpp>
#include <VCL/AST/TypePrinter.hpp>

#include <iostream>


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
        Stmt* body = TransformStmt(functionDecl->GetBody());
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

VCL::TemplateArgumentList* VCL::TemplateInstantiator::TransformTemplateArgumentList(TemplateArgumentList* templateArgs) {
    llvm::SmallVector<TemplateArgument> newTemplateArgs{};
    for (TemplateArgument arg : templateArgs->GetArgs()) {
        switch (arg.GetKind()) {
            case TemplateArgument::Type: {
                QualType type = TransformType(arg.GetType());
                if (type.GetType() == nullptr) 
                    return nullptr;
                newTemplateArgs.push_back(type);
                break;
            }
            case TemplateArgument::Expression: {
                Expr* newExpr = TransformExpr(arg.GetExpr());
                if (!newExpr)
                    return nullptr;
                newTemplateArgs.push_back(newExpr);
                break;
            }
            default: newTemplateArgs.push_back(arg);
        }
    }
    return sema.ActOnTemplateArgumentList(newTemplateArgs, templateArgs->GetSourceRange(), false);
}

VCL::QualType VCL::TemplateInstantiator::TransformType(QualType type) {
    switch (type.GetType()->GetTypeClass()) {
        case Type::TemplateTypeParamTypeClass: return TransformTemplateTypeParamType(type);
        case Type::TemplateSpecializationTypeClass: return TransformTemplateSpecializationType(type);
        case Type::TypeAliasTypeClass: return TransformType(((TypeAliasType*)type.GetType())->GetType());
        default: return type;
    }
}

VCL::Stmt* VCL::TemplateInstantiator::TransformStmt(Stmt* stmt) {
    switch (stmt->GetStmtClass()) {
        case Stmt::ValueStmtClass: return TransformExpr((Expr*)stmt);
        case Stmt::DeclStmtClass: return TransformDeclStmt((DeclStmt*)stmt);
        case Stmt::CompoundStmtClass: return TransformCompoundStmt((CompoundStmt*)stmt);
        case Stmt::ReturnStmtClass: return TransformReturnStmt((ReturnStmt*)stmt);
        case Stmt::IfStmtClass: return TransformIfStmt((IfStmt*)stmt);
        case Stmt::WhileStmtClass: return TransformWhileStmt((WhileStmt*)stmt);
        case Stmt::ForStmtClass: return TransformForStmt((ForStmt*)stmt);
        case Stmt::BreakStmtClass: return TransformBreakStmt((BreakStmt*)stmt);
        case Stmt::ContinueStmtClass: return TransformContinueStmt((ContinueStmt*)stmt);
        default: return stmt;
    }
}

VCL::Decl* VCL::TemplateInstantiator::TransformDecl(Decl* decl) {
    switch (decl->GetDeclClass()) {
        case Decl::FieldDeclClass: return TransformFieldDecl((FieldDecl*)decl);
        case Decl::VarDeclClass: return TransformVarDecl((VarDecl*)decl);
        case Decl::ParamDeclClass: return TransformParamDecl((ParamDecl*)decl);
        default: return decl;
    }
}

VCL::Expr* VCL::TemplateInstantiator::TransformExpr(Expr* expr) {
    switch (expr->GetExprClass())
    {
        case Expr::LoadExprClass: return TransformLoadExpr((LoadExpr*)expr);
        case Expr::DeclRefExprClass: return TransformDeclRefExpr((DeclRefExpr*)expr);
        case Expr::CastExprClass: return TransformCastExpr((CastExpr*)expr);
        case Expr::SplatExprClass: return TransformSplatExpr((SplatExpr*)expr);
        case Expr::BinaryExprClass: return TransformBinaryExpr((BinaryExpr*)expr);
        case Expr::UnaryExprClass: return TransformUnaryExpr((UnaryExpr*)expr);
        case Expr::CallExprClass: return TransformCallExpr((CallExpr*)expr);
        case Expr::DependentCallExprClass: return TransformDependentCallExpr((DependentCallExpr*)expr);
        case Expr::FieldAccessExprClass: return TransformFieldAccessExpr((FieldAccessExpr*)expr);
        case Expr::DependentFieldAccessExprClass: return TransformDependentFieldAccessExpr((DependentFieldAccessExpr*)expr);
        case Expr::SubscriptExprClass: return TransformSubscriptExpr((SubscriptExpr*)expr);
        case Expr::AggregateExprClass: return TransformAggregateExpr((AggregateExpr*)expr);
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
    TemplateArgumentList* arglist = TransformTemplateArgumentList(t->GetTemplateArgumentList());
    if (!t || !arglist)
        return nullptr;
    TemplateSpecializationType* newType = sema.GetASTContext().GetTypeCache().GetOrCreateTemplateSpecializationType(t->GetTemplateDecl(), arglist);
    if (!MakeTypeComplete(newType))
        return nullptr;
    return QualType{ newType, type.GetQualifiers() };
}

VCL::Stmt* VCL::TemplateInstantiator::TransformDeclStmt(DeclStmt* stmt) {
    Decl* decl = TransformDecl(stmt->GetDecl());
    if (!decl)
        return nullptr;
    return sema.ActOnDeclStmt(decl, stmt->GetSourceRange());
}

VCL::Stmt* VCL::TemplateInstantiator::TransformCompoundStmt(CompoundStmt* stmt) {
    llvm::SmallVector<Stmt*> stmts{};
    for (Stmt* stmt : stmt->GetStmts()) {
        Stmt* newStmt = TransformStmt(stmt);
        if (!newStmt)
            return nullptr;
        stmts.push_back(newStmt);
    }
    return sema.ActOnCompoundStmt(stmts, stmt->GetSourceRange());
}

VCL::Stmt* VCL::TemplateInstantiator::TransformReturnStmt(ReturnStmt* stmt) {
    Expr* expr = TransformExpr(stmt->GetExpr());
    if (!expr) {
        return nullptr;
    }
    return sema.ActOnReturnStmt(expr, stmt->GetSourceRange());
}

VCL::Stmt* VCL::TemplateInstantiator::TransformIfStmt(IfStmt* stmt) {
    auto guard = sema.PushScope(nullptr, true);
    
    Expr* condition = TransformExpr(stmt->GetCondition());
    Stmt* thenStmt = TransformStmt(stmt->GetThenStmt());
    Stmt* elseStmt = nullptr;
    if (stmt->GetElseStmt()) {
        elseStmt = TransformStmt(stmt->GetElseStmt());
        if (!elseStmt)
            return nullptr;
    }
    if (!condition || !thenStmt)
        return nullptr;
    return sema.ActOnIfStmt(condition, thenStmt, elseStmt, stmt->GetSourceRange());
}

VCL::Stmt* VCL::TemplateInstantiator::TransformWhileStmt(WhileStmt* stmt) {
    auto guard = sema.PushScope(nullptr, true);

    Expr* condition = TransformExpr(stmt->GetCondition());
    Stmt* thenStmt = TransformStmt(stmt->GetThenStmt());
    if (!condition || !thenStmt)
        return nullptr;
    return sema.ActOnWhileStmt(condition, thenStmt, stmt->GetSourceRange());
}

VCL::Stmt* VCL::TemplateInstantiator::TransformForStmt(ForStmt* stmt) {
    auto guard = sema.PushScope(nullptr, true);

    Stmt* startStmt = nullptr;
    Expr* condition = nullptr;
    Expr* loopExpr = nullptr;
    if (stmt->GetStartStmt()) {
        startStmt = TransformStmt(stmt->GetStartStmt());
        if (!startStmt)
            return nullptr;
    }
    if (stmt->GetCondition()) {
        condition = TransformExpr(stmt->GetCondition());
        if (!condition)
            return nullptr;
    }
    if (stmt->GetLoopExpr()) {
        loopExpr = TransformExpr(stmt->GetLoopExpr());
        if (!loopExpr)
            return nullptr;
    }
    Stmt* thenStmt = TransformStmt(stmt->GetThenStmt());

    if (!thenStmt)
        return nullptr;

    return sema.ActOnForStmt(startStmt, condition, loopExpr, thenStmt, stmt->GetSourceRange());
}

VCL::Stmt* VCL::TemplateInstantiator::TransformBreakStmt(BreakStmt* stmt) {
    return sema.ActOnBreakStmt(stmt->GetSourceRange());
}

VCL::Stmt* VCL::TemplateInstantiator::TransformContinueStmt(ContinueStmt* stmt) {
    return sema.ActOnContinueStmt(stmt->GetSourceRange());
}

VCL::Decl* VCL::TemplateInstantiator::TransformFieldDecl(FieldDecl* decl) {
    QualType type = TransformType(decl->GetType());
    if (type.GetAsOpaquePtr() == 0)
        return nullptr;
    return FieldDecl::Create(sema.GetASTContext(), decl->GetIdentifierInfo(), type, decl->GetSourceRange());
}

VCL::Decl* VCL::TemplateInstantiator::TransformVarDecl(VarDecl* decl) {
    QualType type = TransformType(decl->GetValueType());
    Expr* initializer = nullptr;
    if (decl->GetInitializer())
        initializer = TransformExpr(decl->GetInitializer());
    return sema.ActOnVarDecl(type, decl->GetIdentifierInfo(), decl->GetVarAttrBitfield(), initializer, decl->GetSourceRange());
}

VCL::Decl* VCL::TemplateInstantiator::TransformParamDecl(ParamDecl* decl) {
    QualType type = TransformType(decl->GetValueType());
    return sema.ActOnParamDecl(decl->GetVarAttrBitfield(), type, decl->GetIdentifierInfo(), decl->GetSourceRange());
}

VCL::Expr* VCL::TemplateInstantiator::TransformLoadExpr(LoadExpr* expr) {
    Expr* loadedExpr = TransformExpr(expr->GetExpr());
    return sema.ActOnLoad(loadedExpr);
}

VCL::Expr* VCL::TemplateInstantiator::TransformDeclRefExpr(DeclRefExpr* expr) {
    TemplateArgument* arg = Lookup(expr->GetValueDecl());
    if (!arg && expr->GetValueDecl()->IsExported())
        return expr;
    else if (!arg)
        return sema.ActOnIdentifierExpr(expr->GetValueDecl()->GetIdentifierInfo(), expr->GetSourceRange());
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
    QualType toType = TransformType(expr->GetResultType());
    if (arg == expr->GetExpr())
        return expr;
    return sema.ActOnCast(arg, toType, expr->GetSourceRange());
}

VCL::Expr* VCL::TemplateInstantiator::TransformSplatExpr(SplatExpr* expr) {
    Expr* broadcastedExpr = TransformExpr(expr->GetExpr());
    return sema.ActOnSplat(broadcastedExpr, expr->GetSourceRange());
}

VCL::Expr* VCL::TemplateInstantiator::TransformBinaryExpr(BinaryExpr* expr) {
    Expr* lhs = TransformExpr(expr->GetLHS());
    Expr* rhs = TransformExpr(expr->GetRHS());
    if (!lhs || !rhs)
        return nullptr;
    return sema.ActOnBinaryExpr(lhs, rhs, expr->GetOperatorKind());
}

VCL::Expr* VCL::TemplateInstantiator::TransformUnaryExpr(UnaryExpr* expr) {
    Expr* arg = TransformExpr(expr->GetExpr());
    if (!arg)
        return nullptr;
    return sema.ActOnUnaryExpr(arg, expr->GetOperator(), expr->GetSourceRange());
}

VCL::Expr* VCL::TemplateInstantiator::TransformCallExpr(CallExpr* expr) {
    llvm::SmallVector<Expr*> args{};
    for (auto arg : expr->GetArgs()) {
        Expr* newArg = TransformExpr(arg);
        if (!newArg)
            return nullptr;
        args.push_back(newArg);
    }

    return sema.ActOnCallExpr(expr->GetFunctionDecl()->GetIdentifierInfo(), args, nullptr, expr->GetSourceRange());
}

VCL::Expr* VCL::TemplateInstantiator::TransformDependentCallExpr(DependentCallExpr* expr) {
    llvm::SmallVector<Expr*> args{};
    for (auto arg : expr->GetArgs()) {
        Expr* newArg = TransformExpr(arg);
        if (!newArg)
            return nullptr;
        args.push_back(newArg);
    }
    TemplateArgumentList* templateArgs = nullptr;
    if (expr->GetTemplateArgs() != nullptr) {
        templateArgs = TransformTemplateArgumentList(expr->GetTemplateArgs());
        if (!templateArgs)
            return nullptr;
    }
    return sema.ActOnCallExpr(expr->GetSymbolRef(), args, templateArgs, expr->GetSourceRange());
}

VCL::Expr* VCL::TemplateInstantiator::TransformFieldAccessExpr(FieldAccessExpr* expr) {
    Expr* arg = TransformExpr(expr->GetExpr());
    if (!arg)
        return nullptr;
    return FieldAccessExpr::Create(sema.GetASTContext(), arg, expr->GetRecordType(), expr->GetFieldIndex(), expr->GetResultType(), expr->GetSourceRange());
}

VCL::Expr* VCL::TemplateInstantiator::TransformDependentFieldAccessExpr(DependentFieldAccessExpr* expr) {
    Expr* arg = TransformExpr(expr->GetExpr());
    if (!arg)
        return nullptr;
    return sema.ActOnFieldAccessExpr(arg, expr->GetField(), expr->GetSourceRange());
}

VCL::Expr* VCL::TemplateInstantiator::TransformSubscriptExpr(SubscriptExpr* expr) {
    Expr* arg = TransformExpr(expr->GetExpr());
    Expr* index = TransformExpr(expr->GetIndex());
    if (!arg || !index)
        return nullptr;
    return sema.ActOnSubscriptExpr(arg, index, expr->GetSourceRange());
}

VCL::Expr* VCL::TemplateInstantiator::TransformAggregateExpr(AggregateExpr* expr) {
    llvm::SmallVector<Expr*> elements{};

    for (auto element : expr->GetElements()) {
        Expr* newElement = TransformExpr(element);
        if (!newElement)
            return nullptr;
        elements.push_back(newElement);
    }

    return sema.ActOnAggregateExpr(elements, expr->GetSourceRange());
}