#pragma once

#include <VCL/AST/ASTContext.hpp>
#include <VCL/AST/Template.hpp>
#include <VCL/AST/DeclTemplate.hpp>

#include <llvm/ADT/DenseMap.h>


namespace VCL {
    class Sema;

    class TemplateInstantiator {
    public:
        TemplateInstantiator() = delete;
        TemplateInstantiator(Sema& sema) : sema{ sema }, substitutionTable{} {}
        TemplateInstantiator(const TemplateInstantiator& other) = delete;
        TemplateInstantiator(TemplateInstantiator&& other) = delete;
        ~TemplateInstantiator() = default;
        
        TemplateInstantiator& operator=(const TemplateInstantiator& other) = delete;
        TemplateInstantiator& operator=(TemplateInstantiator&& other) = delete;
        
        bool MakeTypeComplete(Type* type);
        bool InstantiateTemplateSpecializationType(TemplateSpecializationType* type);
        bool AddTemplateArgumentListAndDecl(TemplateArgumentList* args, TemplateDecl* decl);
        bool CheckTemplateArgumentsParametersMatch(TemplateArgumentList* args, TemplateParameterList* params);
        bool EvaluateTemplateArgumentsExpr(TemplateArgumentList* args);

        Type* InstantiateTemplatedIntrinsicTypeDecl(TemplateDecl* decl);
        Type* InstantiateTemplatedRecordDecl(TemplateDecl* decl);
        Type* InstantiateTemplatedTypeAliasDecl(TemplateDecl* decl);
        FunctionDecl* InstantiateTemplatedFunctionDecl(TemplateDecl* decl);

        void AddSubstitution(NamedDecl* param, TemplateArgument* arg);
        TemplateArgument* Lookup(NamedDecl* param);

        TemplateArgumentList* TransformTemplateArgumentList(TemplateArgumentList* templateArgs);
        QualType TransformType(QualType type);
        Stmt* TransformStmt(Stmt* stmt);
        Decl* TransformDecl(Decl* decl);
        Expr* TransformExpr(Expr* expr);

        // Transform Type

        QualType TransformTemplateTypeParamType(QualType type);
        QualType TransformTemplateSpecializationType(QualType type);

        // Transform Stmt

        Stmt* TransformDeclStmt(DeclStmt* stmt);
        Stmt* TransformCompoundStmt(CompoundStmt* stmt);
        Stmt* TransformReturnStmt(ReturnStmt* stmt);
        Stmt* TransformIfStmt(IfStmt* stmt);
        Stmt* TransformWhileStmt(WhileStmt* stmt);
        Stmt* TransformForStmt(ForStmt* stmt);
        Stmt* TransformBreakStmt(BreakStmt* stmt);
        Stmt* TransformContinueStmt(ContinueStmt* stmt);

        // Transform Decl

        Decl* TransformFieldDecl(FieldDecl* decl);
        Decl* TransformVarDecl(VarDecl* decl);
        Decl* TransformParamDecl(ParamDecl* decl);

        // Transform Expr

        Expr* TransformLoadExpr(LoadExpr* expr);
        Expr* TransformDeclRefExpr(DeclRefExpr* expr);
        Expr* TransformCastExpr(CastExpr* expr);
        Expr* TransformSplatExpr(SplatExpr* expr);
        Expr* TransformBinaryExpr(BinaryExpr* expr);
        Expr* TransformUnaryExpr(UnaryExpr* expr);
        Expr* TransformCallExpr(CallExpr* expr);
        Expr* TransformDependentCallExpr(DependentCallExpr* expr);
        Expr* TransformFieldAccessExpr(FieldAccessExpr* expr);
        Expr* TransformDependentFieldAccessExpr(DependentFieldAccessExpr* expr);
        Expr* TransformSubscriptExpr(SubscriptExpr* expr);
        Expr* TransformAggregateExpr(AggregateExpr* expr);

    private:
        Sema& sema;
        llvm::DenseMap<NamedDecl*, TemplateArgument*> substitutionTable;
    };

}