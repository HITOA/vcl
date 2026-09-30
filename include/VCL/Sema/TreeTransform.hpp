#pragma once

#include <VCL/AST/ASTContext.hpp>
#include <VCL/AST/Template.hpp>
#include <VCL/AST/Decl.hpp>
#include <VCL/AST/DeclTemplate.hpp>
#include <VCL/AST/Stmt.hpp>
#include <VCL/AST/Expr.hpp>

#include <llvm/ADT/DenseMap.h>


namespace VCL {
    class Sema;

    /**
     * Rebuilds declarations, statements and expressions through Sema's ActOn*, so that what it
     * produces is checked again (types, lvalues, const). Every rebuilt node keeps the source range
     * of the node it comes from.
     *
     * Each Transform* is a hook: a subclass overrides the ones its rewrite needs and calls the base
     * for the rest (TemplateInstantiator substitutes template parameters; a lowering step could
     * redirect references to a variable).
     *
     * Declarations the transform creates (functions, variables, parameters, locals) are recorded,
     * old -> new, and references to a recorded declaration are rebuilt against its copy. Anything
     * else keeps the declaration it was bound to where it was written: names are never looked up
     * again.
     *
     * New declarations go in Sema's current scope and context: enter the scope they belong in first
     * (e.g. Sema::EnterTranslationUnit).
     */
    class TreeTransform {
    public:
        TreeTransform() = delete;
        TreeTransform(Sema& sema) : sema{ sema } {}
        TreeTransform(const TreeTransform& other) = delete;
        TreeTransform(TreeTransform&& other) = delete;
        virtual ~TreeTransform() = default;

        TreeTransform& operator=(const TreeTransform& other) = delete;
        TreeTransform& operator=(TreeTransform&& other) = delete;

        inline Sema& GetSema() { return sema; }

        /** Records that references to `from` are to be rebuilt against `to`. */
        inline void MapDecl(Decl* from, Decl* to) { transformedDecls[from] = to; }
        /** The copy of `decl` made (or mapped) by this transform, or nullptr. */
        Decl* GetTransformedDecl(Decl* decl) const;

        virtual TemplateArgumentList* TransformTemplateArgumentList(TemplateArgumentList* templateArgs);
        virtual QualType TransformType(QualType type);
        virtual Stmt* TransformStmt(Stmt* stmt);
        virtual Decl* TransformDecl(Decl* decl);
        virtual Expr* TransformExpr(Expr* expr);

        // Transform Type (each returns the type unchanged by default)

        virtual QualType TransformTemplateTypeParamType(QualType type);
        virtual QualType TransformTemplateSpecializationType(QualType type);
        virtual QualType TransformTypeAliasType(QualType type);

        // Transform Stmt

        virtual Stmt* TransformDeclStmt(DeclStmt* stmt);
        virtual Stmt* TransformCompoundStmt(CompoundStmt* stmt);
        virtual Stmt* TransformReturnStmt(ReturnStmt* stmt);
        virtual Stmt* TransformIfStmt(IfStmt* stmt);
        virtual Stmt* TransformWhileStmt(WhileStmt* stmt);
        virtual Stmt* TransformForStmt(ForStmt* stmt);
        virtual Stmt* TransformBreakStmt(BreakStmt* stmt);
        virtual Stmt* TransformContinueStmt(ContinueStmt* stmt);

        // Transform Decl

        virtual Decl* TransformFieldDecl(FieldDecl* decl);
        virtual Decl* TransformVarDecl(VarDecl* decl);
        virtual Decl* TransformParamDecl(ParamDecl* decl);
        /**
         * A function with the same name, attributes, parameters and body, declared in the current
         * scope and context. Intrinsics and bodiless declarations are rebuilt without a body.
         */
        virtual Decl* TransformFunctionDecl(FunctionDecl* decl);
        /** The body of `function`, rebuilt in the scope of the function being built (already pushed). */
        virtual Stmt* TransformFunctionBody(FunctionDecl* function);
        /** Copies the attributes of `from` onto `to`. */
        virtual void TransformAttributes(Decl* from, Decl* to);

        // Transform Expr

        virtual Expr* TransformLoadExpr(LoadExpr* expr);
        virtual Expr* TransformDeclRefExpr(DeclRefExpr* expr);
        virtual Expr* TransformCastExpr(CastExpr* expr);
        virtual Expr* TransformSplatExpr(SplatExpr* expr);
        virtual Expr* TransformBinaryExpr(BinaryExpr* expr);
        virtual Expr* TransformUnaryExpr(UnaryExpr* expr);
        virtual Expr* TransformCallExpr(CallExpr* expr);
        virtual Expr* TransformDependentCallExpr(DependentCallExpr* expr);
        virtual Expr* TransformFieldAccessExpr(FieldAccessExpr* expr);
        virtual Expr* TransformDependentFieldAccessExpr(DependentFieldAccessExpr* expr);
        virtual Expr* TransformSubscriptExpr(SubscriptExpr* expr);
        virtual Expr* TransformAggregateExpr(AggregateExpr* expr);

    protected:
        Sema& sema;
        // Declarations created (or mapped) by the transform -> their copy.
        llvm::DenseMap<Decl*, Decl*> transformedDecls{};
        // The body of the function being rebuilt: unlike other blocks, it has no scope of its own.
        Stmt* functionBody = nullptr;
    };

}
