#include <VCL/Sema/TreeTransform.hpp>

#include <optional>

#include <VCL/Core/Diagnostic.hpp>
#include <VCL/AST/AttributeInstance.hpp>
#include <VCL/Sema/Sema.hpp>


VCL::Decl* VCL::TreeTransform::GetTransformedDecl(Decl* decl) const {
    auto it = transformedDecls.find(decl);
    return it != transformedDecls.end() ? it->second : nullptr;
}

VCL::TemplateArgumentList* VCL::TreeTransform::TransformTemplateArgumentList(TemplateArgumentList* templateArgs) {
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

VCL::QualType VCL::TreeTransform::TransformType(QualType type) {
    switch (type.GetType()->GetTypeClass()) {
        case Type::TemplateTypeParamTypeClass: return TransformTemplateTypeParamType(type);
        case Type::TemplateSpecializationTypeClass: return TransformTemplateSpecializationType(type);
        case Type::TypeAliasTypeClass: return TransformTypeAliasType(type);
        default: return type;
    }
}

VCL::Stmt* VCL::TreeTransform::TransformStmt(Stmt* stmt) {
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

VCL::Decl* VCL::TreeTransform::TransformDecl(Decl* decl) {
    switch (decl->GetDeclClass()) {
        case Decl::FieldDeclClass: return TransformFieldDecl((FieldDecl*)decl);
        case Decl::VarDeclClass: return TransformVarDecl((VarDecl*)decl);
        case Decl::ParamDeclClass: return TransformParamDecl((ParamDecl*)decl);
        case Decl::FunctionDeclClass: return TransformFunctionDecl((FunctionDecl*)decl);
        default: return decl;
    }
}

VCL::Expr* VCL::TreeTransform::TransformExpr(Expr* expr) {
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

VCL::QualType VCL::TreeTransform::TransformTemplateTypeParamType(QualType type) {
    return type;
}

VCL::QualType VCL::TreeTransform::TransformTemplateSpecializationType(QualType type) {
    return type;
}

VCL::QualType VCL::TreeTransform::TransformTypeAliasType(QualType type) {
    return type;
}

VCL::Stmt* VCL::TreeTransform::TransformDeclStmt(DeclStmt* stmt) {
    Decl* decl = TransformDecl(stmt->GetDecl());
    if (!decl)
        return nullptr;
    return sema.ActOnDeclStmt(decl, stmt->GetSourceRange());
}

VCL::Stmt* VCL::TreeTransform::TransformCompoundStmt(CompoundStmt* stmt) {
    std::optional<Sema::SemaScopeGuard> guard{};
    if (stmt != functionBody)
        guard.emplace(sema.PushScope(nullptr, false));
    llvm::SmallVector<Stmt*> stmts{};
    for (Stmt* stmt : stmt->GetStmts()) {
        Stmt* newStmt = TransformStmt(stmt);
        if (!newStmt)
            return nullptr;
        stmts.push_back(newStmt);
    }
    return sema.ActOnCompoundStmt(stmts, stmt->GetSourceRange());
}

VCL::Stmt* VCL::TreeTransform::TransformReturnStmt(ReturnStmt* stmt) {
    Expr* expr = nullptr;
    if (stmt->GetExpr() != nullptr) { // `return;` has none
        expr = TransformExpr(stmt->GetExpr());
        if (!expr)
            return nullptr;
    }
    return sema.ActOnReturnStmt(expr, stmt->GetSourceRange());
}

VCL::Stmt* VCL::TreeTransform::TransformIfStmt(IfStmt* stmt) {
    auto guard = sema.PushScope(nullptr, false);

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

VCL::Stmt* VCL::TreeTransform::TransformWhileStmt(WhileStmt* stmt) {
    auto guard = sema.PushScope(nullptr, true);

    Expr* condition = TransformExpr(stmt->GetCondition());
    Stmt* thenStmt = TransformStmt(stmt->GetThenStmt());
    if (!condition || !thenStmt)
        return nullptr;
    return sema.ActOnWhileStmt(condition, thenStmt, stmt->GetSourceRange());
}

VCL::Stmt* VCL::TreeTransform::TransformForStmt(ForStmt* stmt) {
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

VCL::Stmt* VCL::TreeTransform::TransformBreakStmt(BreakStmt* stmt) {
    return sema.ActOnBreakStmt(stmt->GetSourceRange());
}

VCL::Stmt* VCL::TreeTransform::TransformContinueStmt(ContinueStmt* stmt) {
    return sema.ActOnContinueStmt(stmt->GetSourceRange());
}

VCL::Decl* VCL::TreeTransform::TransformFieldDecl(FieldDecl* decl) {
    QualType type = TransformType(decl->GetType());
    if (type.GetAsOpaquePtr() == 0)
        return nullptr;
    return FieldDecl::Create(sema.GetASTContext(), decl->GetIdentifierInfo(), type, decl->GetSourceRange());
}

VCL::Decl* VCL::TreeTransform::TransformVarDecl(VarDecl* decl) {
    QualType type = TransformType(decl->GetValueType());
    if (!type.GetAsOpaquePtr())
        return nullptr;
    Expr* initializer = nullptr;
    if (decl->GetInitializer()) {
        initializer = TransformExpr(decl->GetInitializer());
        if (!initializer)
            return nullptr;
    }
    VarDecl* newDecl = sema.ActOnVarDecl(type, decl->GetIdentifierInfo(), decl->GetVarAttrBitfield(), initializer, decl->GetSourceRange());
    if (!newDecl)
        return nullptr;
    TransformAttributes(decl, newDecl);
    newDecl->SetExported(decl->IsExported());
    MapDecl(decl, newDecl);
    return newDecl;
}

VCL::Decl* VCL::TreeTransform::TransformParamDecl(ParamDecl* decl) {
    // ActOnParamDecl makes aggregates and `out` parameters references: rebuild from the type as
    // written, or it would be wrapped a second time.
    QualType type = decl->GetValueType();
    if (type.GetType()->GetTypeClass() == Type::ReferenceTypeClass)
        type = ((ReferenceType*)type.GetType())->GetType();
    type = TransformType(type);
    if (!type.GetAsOpaquePtr())
        return nullptr;
    ParamDecl* newDecl = sema.ActOnParamDecl(decl->GetVarAttrBitfield(), type, decl->GetIdentifierInfo(), decl->GetSourceRange());
    if (!newDecl)
        return nullptr;
    newDecl->SetCodeGenFlag(decl->GetCodeGenFlags());
    MapDecl(decl, newDecl);
    return newDecl;
}

VCL::Decl* VCL::TreeTransform::TransformFunctionDecl(FunctionDecl* decl) {
    FunctionDecl* newDecl = FunctionDecl::Create(sema.GetASTContext(), decl->GetIdentifierInfo());
    if (decl->HasFunctionFlag(FunctionDecl::IsIntrinsic)) {
        newDecl->SetIntrinsicID(decl->GetIntrinsicID());
        newDecl->SetFunctionFlag(FunctionDecl::IsIntrinsic);
    }

    QualType returnType = TransformType(decl->GetType()->GetReturnType());
    if (!returnType.GetAsOpaquePtr())
        return nullptr;

    {
        auto guard = sema.PushScope(newDecl, false);
        if (!TransformFunctionParams(decl, newDecl))
            return nullptr;
    }

    // As the parser does: declared in the enclosing scope once its parameters are known.
    if (!sema.ActOnFunctionDecl(newDecl, returnType, nullptr, decl->GetSourceRange()))
        return nullptr;
    TransformAttributes(decl, newDecl);
    newDecl->SetExported(decl->IsExported());
    MapDecl(decl, newDecl);

    if (decl->GetBody() != nullptr && !decl->HasFunctionFlag(FunctionDecl::IsIntrinsic)) {
        auto guard = sema.PushScope(newDecl, false);
        Stmt* body = TransformFunctionBody(decl);
        if (!body)
            return nullptr;
        newDecl->SetBody(body);
    }

    return newDecl;
}

bool VCL::TreeTransform::TransformFunctionParams(FunctionDecl* from, FunctionDecl* to) {
    for (auto it = from->Begin(); it != from->End(); ++it) {
        if (it->GetDeclClass() != Decl::ParamDeclClass)
            continue;
        if (!TransformDecl(it.Get()))
            return false;
    }
    return true;
}

VCL::Stmt* VCL::TreeTransform::TransformFunctionBody(FunctionDecl* function) {
    Stmt* enclosingBody = functionBody;
    functionBody = function->GetBody();
    Stmt* body = TransformStmt(function->GetBody());
    functionBody = enclosingBody;
    return body;
}

void VCL::TreeTransform::TransformAttributes(Decl* from, Decl* to) {
    // Instances are chained through themselves: each declaration needs its own.
    for (AttributeInstance* attribute = from->GetAttribute(); attribute != nullptr; attribute = attribute->GetNextAttribute())
        to->PushAttribute(AttributeInstance::Create(sema.GetASTContext(), attribute->GetDefinition(), attribute->GetArgs(), attribute->GetSourceRange()));
}

VCL::Expr* VCL::TreeTransform::TransformLoadExpr(LoadExpr* expr) {
    Expr* loadedExpr = TransformExpr(expr->GetExpr());
    return sema.ActOnLoad(loadedExpr);
}

VCL::Expr* VCL::TreeTransform::TransformDeclRefExpr(DeclRefExpr* expr) {
    // A declaration this transform rebuilt: refer to its copy. Anything else was bound where the
    // code was written; looking the name up again here would resolve it in the current scope.
    Decl* decl = GetTransformedDecl(expr->GetValueDecl());
    return sema.ActOnDeclRefExpr(decl ? (ValueDecl*)decl : expr->GetValueDecl(), expr->GetSourceRange());
}

VCL::Expr* VCL::TreeTransform::TransformCastExpr(CastExpr* expr) {
    Expr* arg = TransformExpr(expr->GetExpr());
    if (!arg)
        return nullptr;
    QualType toType = TransformType(expr->GetResultType());
    if (!toType.GetAsOpaquePtr())
        return nullptr;
    // A cast is rebuilt even when its operand didn't change: the target type may have.
    if (arg == expr->GetExpr() && expr->GetCastKind() != CastExpr::Dependent && toType == expr->GetResultType())
        return expr;
    return sema.ActOnExplicitCast(arg, toType, expr->GetSourceRange());
}

VCL::Expr* VCL::TreeTransform::TransformSplatExpr(SplatExpr* expr) {
    Expr* broadcastedExpr = TransformExpr(expr->GetExpr());
    return sema.ActOnSplat(broadcastedExpr, expr->GetSourceRange());
}

VCL::Expr* VCL::TreeTransform::TransformBinaryExpr(BinaryExpr* expr) {
    Expr* lhs = TransformExpr(expr->GetLHS());
    Expr* rhs = TransformExpr(expr->GetRHS());
    if (!lhs || !rhs)
        return nullptr;
    return sema.ActOnBinaryExpr(lhs, rhs, expr->GetOperatorKind());
}

VCL::Expr* VCL::TreeTransform::TransformUnaryExpr(UnaryExpr* expr) {
    Expr* arg = TransformExpr(expr->GetExpr());
    if (!arg)
        return nullptr;
    return sema.ActOnUnaryExpr(arg, expr->GetOperator(), expr->GetSourceRange());
}

VCL::Expr* VCL::TreeTransform::TransformCallExpr(CallExpr* expr) {
    llvm::SmallVector<Expr*> args{};
    for (auto arg : expr->GetArgs()) {
        Expr* newArg = TransformExpr(arg);
        if (!newArg)
            return nullptr;
        args.push_back(newArg);
    }

    // The callee was bound where the code was written; don't look its name up again. A callee
    // this transform rebuilt is called through its copy.
    FunctionDecl* callee = expr->GetFunctionDecl();
    if (Decl* transformed = GetTransformedDecl(callee))
        callee = (FunctionDecl*)transformed;
    return sema.ActOnResolvedCallExpr(callee, callee->GetIdentifierInfo(), args, nullptr, expr->GetSourceRange());
}

VCL::Expr* VCL::TreeTransform::TransformDependentCallExpr(DependentCallExpr* expr) {
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
    if (Decl* callee = expr->GetResolvedCallee())
        return sema.ActOnResolvedCallExpr(callee, expr->GetSymbolRef(), args, templateArgs, expr->GetSourceRange());
    return sema.ActOnCallExpr(expr->GetSymbolRef(), args, templateArgs, expr->GetSourceRange());
}

VCL::Expr* VCL::TreeTransform::TransformFieldAccessExpr(FieldAccessExpr* expr) {
    Expr* arg = TransformExpr(expr->GetExpr());
    if (!arg)
        return nullptr;
    return FieldAccessExpr::Create(sema.GetASTContext(), arg, expr->GetRecordType(), expr->GetFieldIndex(), expr->GetResultType(), expr->GetSourceRange());
}

VCL::Expr* VCL::TreeTransform::TransformDependentFieldAccessExpr(DependentFieldAccessExpr* expr) {
    Expr* arg = TransformExpr(expr->GetExpr());
    if (!arg)
        return nullptr;
    return sema.ActOnFieldAccessExpr(arg, expr->GetField(), expr->GetSourceRange());
}

VCL::Expr* VCL::TreeTransform::TransformSubscriptExpr(SubscriptExpr* expr) {
    Expr* arg = TransformExpr(expr->GetExpr());
    Expr* index = TransformExpr(expr->GetIndex());
    if (!arg || !index)
        return nullptr;
    return sema.ActOnSubscriptExpr(arg, index, expr->GetSourceRange());
}

VCL::Expr* VCL::TreeTransform::TransformAggregateExpr(AggregateExpr* expr) {
    llvm::SmallVector<Expr*> elements{};

    for (auto element : expr->GetElements()) {
        Expr* newElement = TransformExpr(element);
        if (!newElement)
            return nullptr;
        elements.push_back(newElement);
    }

    return sema.ActOnAggregateExpr(elements, expr->GetSourceRange());
}
