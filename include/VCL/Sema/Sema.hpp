#pragma once

#include <VCL/Lex/Token.hpp>
#include <VCL/AST/ASTContext.hpp>
#include <VCL/AST/Type.hpp>
#include <VCL/AST/Decl.hpp>
#include <VCL/AST/DeclTemplate.hpp>
#include <VCL/AST/Stmt.hpp>
#include <VCL/AST/Expr.hpp>
#include <VCL/AST/Operator.hpp>
#include <VCL/AST/Template.hpp>
#include <VCL/Sema/Scope.hpp>
#include <VCL/Sema/ScopeManager.hpp>

#include <stack>


namespace VCL {
    class DiagnosticReporter;
    class IdentifierTable;

    /**
     * This is the semantic analyzer. It is called by the parser to 
     * act on each semantic and build the ast. 
     */
    class Sema {
    public:
        Sema() = delete;
        Sema(ASTContext& astContext, DiagnosticReporter& diagnosticReporter, IdentifierTable& identifierTable);
        Sema(const Sema& other) = delete;
        Sema(Sema&& other) = delete;
        ~Sema() = default;

        Sema& operator=(const Sema& other) = delete;
        Sema& operator=(Sema&& other) = delete;

    public:
        inline ASTContext& GetASTContext() { return astContext; }
        inline DiagnosticReporter& GetDiagnosticReporter() { return diagnosticReporter; }
        inline IdentifierTable& GetIdentifierTable() { return identifierTable; }

        void AddBuiltinIntrinsicTemplateDecl();

        bool PushDeclContextScope(DeclContext* context);
        bool PopDeclContextScope(DeclContext* context);

        CompoundStmt* ActOnCompoundStmt(llvm::ArrayRef<Stmt*> stmts, SourceRange range);

        DeclStmt* ActOnDeclStmt(Decl* decl, SourceRange range);

        RecordDecl* ActOnRecordDecl(IdentifierInfo* identifier, SourceRange range);
        TemplateRecordDecl* ActOnTemplateRecordDecl(IdentifierInfo* identifier, TemplateParameterList* params, SourceRange range);
        FieldDecl* ActOnFieldDecl(QualType type, IdentifierInfo* identifier, SourceRange range);

        TransientFunctionDecl* ActOnTransientFunctionDecl(QualType returnType, IdentifierInfo* identifier, SourceRange range);
        FunctionDecl* ActOnFunctionDecl(TransientFunctionDecl* decl, SourceRange range);
        ParamDecl* ActOnParamDecl(VarDecl::VarAttrBitfield attr, QualType type, IdentifierInfo* identifier, SourceRange range);
        CompoundStmt* ActOnFunctionBody(FunctionDecl* function, CompoundStmt* body);
        bool ActRecOnFunctionBodyReturnStmt(FunctionDecl* function, llvm::ArrayRef<Stmt*> stmts);

        ReturnStmt* ActOnReturnStmt(Expr* expr, SourceRange range);

        VarDecl* ActOnVarDecl(QualType type, IdentifierInfo* identifier, VarDecl::VarAttrBitfield varAttrBitfield, Expr* initializer, SourceRange range);
        QualType ActOnQualType(Type* type, Qualifier qualifiers, SourceRange range);
        Type* ActOnType(IdentifierInfo* identifier, TemplateArgumentList* list, SourceRange range);
        
        TemplateParameterList* ActOnTemplateParameterList(llvm::ArrayRef<NamedDecl*> params, SourceRange range);
        TemplateArgumentList* ActOnTemplateArgumentList(llvm::ArrayRef<TemplateArgument> args, SourceRange range);

        TemplateTypeParamDecl* ActOnTemplateTypeParamDecl(IdentifierInfo* identifier, SourceRange range);
        NonTypeTemplateParamDecl* ActOnNonTypeTemplateParamDecl(BuiltinType* type, IdentifierInfo* identifier, SourceRange range);

        Expr* ActOnBinaryExpr(Expr* lhs, Expr* rhs, BinaryOperator::Kind op);
        Expr* ActOnUnaryExpr(Expr* expr, UnaryOperator op, SourceRange range);
        bool IsExprAssignable(Expr* expr);

        Expr* ActOnFieldAccessExpr(Expr* lhs, IdentifierInfo* field, SourceRange range);
        Expr* ActOnSubscriptExpr(Expr* expr, Expr* index, SourceRange range);

        Expr* ActOnLoad(Expr* expr);

        std::pair<Expr*, Expr*> ActOnImplicitBinaryArithmeticCast(Expr* lhs, Expr* rhs);
        Expr* ActOnCast(Expr* expr, QualType toType, SourceRange range);
        Expr* ActOnSplat(Expr* expr, SourceRange range);

        Expr* ActOnNumericConstant(Token* value);
        Expr* ActOnIdentifierExpr(IdentifierInfo* identifier, SourceRange range);
        Expr* ActOnCallExpr(IdentifierInfo* identifier, llvm::ArrayRef<Expr*> args, SourceRange range);
        Expr* ActOnAggregateExpr(llvm::ArrayRef<Expr*> elems, SourceRange range);
        Expr* ActOnAggregateExpr(QualType type, AggregateExpr* aggregate);

        NamedDecl* LookupNamedDecl(IdentifierInfo* identifier, int depth = -1);
        IntrinsicTemplateDecl* LookupIntrinsicTemplateDecl(IdentifierInfo* identifier, int depth = -1);
        VarDecl* LookupVarDecl(IdentifierInfo* identifier, int depth = -1);
        FunctionDecl* LookupFunctionDecl(IdentifierInfo* identifier, int depth = -1);
        
        FunctionDecl* GetFrontmostFunctionDecl();

        bool TypePreferByReference(Type* type);
        Type* GetInstantiatedType(Type* type);
        bool CheckTypeCastability(Type* type);
        BuiltinType::Kind GetScalarKindFromBuiltinOrVectorType(Type* type);
        bool IsCurrentScopeGlobal();

    private:
        ASTContext& astContext;
        DiagnosticReporter& diagnosticReporter;
        IdentifierTable& identifierTable;
        ScopeManager sm{};
    };

}