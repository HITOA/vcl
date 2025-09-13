#pragma once

#include <VCL/Core/CompilerContext.hpp>
#include <VCL/Core/Identifier.hpp>
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

    /**
     * This is the semantic analyzer. It is called by the parser to 
     * act on each semantic and build the ast. 
     */
    class Sema {
    public:
        Sema() = delete;
        Sema(ASTContext& astContext, CompilerContext& cc);
        Sema(const Sema& other) = delete;
        Sema(Sema&& other) = delete;
        ~Sema() = default;

        Sema& operator=(const Sema& other) = delete;
        Sema& operator=(Sema&& other) = delete;

    public:
        inline ASTContext& GetASTContext() { return astContext; }

        void AddBuiltinIntrinsicTemplateDecl();

        VarDecl* ActOnVarDecl(QualType type, IdentifierInfo* identifier, VarDecl::VarAttrBitfield varAttrBitfield, Expr* initializer, SourceRange range);
        QualType ActOnQualType(Type* type, Qualifier qualifiers, SourceRange range);
        Type* ActOnType(IdentifierInfo* identifier, TemplateArgumentList* list, SourceRange range);
        
        TemplateArgumentList* ActOnTemplateArgumentList(llvm::ArrayRef<TemplateArgument> args, SourceRange range);

        Expr* ActOnBinaryExpr(Expr* lhs, Expr* rhs, BinaryOperator& op);

        Expr* ActOnBinaryAssignmentExpr(Expr* lhs, Expr* rhs, BinaryOperator& op);
        Expr* ActOnBinaryLogicalExpr(Expr* lhs, Expr* rhs, BinaryOperator& op);
        Expr* ActOnBinaryArithmeticExpr(Expr* lhs, Expr* rhs, BinaryOperator& op);

        std::pair<Expr*, Expr*> ActOnImplicitBinaryArithmeticCast(Expr* lhs, Expr* rhs);
        Expr* ActOnCast(Expr* expr, QualType toType, SourceRange range);

        Expr* ActOnNumericConstant(Token* value);
        Expr* ActOnIdentifierExpr(IdentifierInfo* identifier, SourceRange range);

        NamedDecl* LookupNamedDecl(IdentifierInfo* identifier, int depth = -1);
        TypeDecl* LookupTypeDecl(IdentifierInfo* identifier, int depth = -1);
        IntrinsicTemplateDecl* LookupIntrinsicTemplateDecl(IdentifierInfo* identifier, int depth = -1);
        VarDecl* LookupVarDecl(IdentifierInfo* identifier, int depth = -1);

        bool CheckTypeCastability(Type* type);

    private:
        ASTContext& astContext;
        CompilerContext& cc;
        ScopeManager sm{};
    };

}