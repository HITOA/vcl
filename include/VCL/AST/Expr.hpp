#pragma once

#include <VCL/AST/Type.hpp>
#include <VCL/AST/Stmt.hpp>
#include <VCL/AST/Decl.hpp>
#include <VCL/AST/ConstantValue.hpp>
#include <VCL/AST/Operator.hpp>
#include <VCL/AST/ASTContext.hpp>


namespace VCL {

    class Expr : ValueStmt {
    public:
        enum ExprClass {
            NumericLiteralExprClass,
            DeclRefExprClass,
            CastExprClass,
            BinaryArithmeticExprClass
        };

    public:
        Expr() = delete;
        Expr(ExprClass exprClass) : exprClass{ exprClass }, resultType{}, range{}, constantValue{ } {}
        Expr(ExprClass exprClass, QualType resultType) : exprClass{ exprClass }, resultType{ resultType }, range{}, constantValue{ } {}
        Expr(const Expr& other) = delete;
        Expr(Expr&& other) = delete;
        ~Expr() = default;

        Expr& operator=(const Expr& other) = delete;
        Expr& operator=(Expr&& other) = delete;

        inline ExprClass GetExprClass() const { return exprClass; }
        inline QualType GetResultType() const { return resultType; }
        inline void SetResultType(QualType resultType) { this->resultType = resultType; }
        inline SourceRange GetSourceRange() const { return range; }
        inline void SetSourceRange(SourceRange range) { this->range = range; }
        inline ConstantValue* GetConstantValue() const { return constantValue; }
        inline void SetConstantValue(ConstantValue* constantValue) { this->constantValue = constantValue; }

    private:
        ExprClass exprClass;
        QualType resultType;
        SourceRange range;
        ConstantValue* constantValue;
    };

    class NumericLiteralExpr : public Expr {
    public:
        NumericLiteralExpr(ConstantScalar scalar) : scalar{ scalar }, Expr{ Expr::NumericLiteralExprClass } {
            SetConstantValue(&scalar);
        }
        ~NumericLiteralExpr() = default;

        inline ConstantScalar* GetValue() { return &scalar; }

        static inline NumericLiteralExpr* Create(ASTContext& context, ConstantScalar scalar, SourceRange range) {
            NumericLiteralExpr* instance = context.AllocateNode<NumericLiteralExpr>(scalar);
            BuiltinType* resultType = context.GetTypeCache().GetOrCreateBuiltinType(instance->scalar.GetKind());
            instance->SetResultType(resultType);
            instance->SetSourceRange(range);
            return instance;
        }

    private:
        ConstantScalar scalar;
    };

    class DeclRefExpr : public Expr {
    public:
        DeclRefExpr(ValueDecl* decl) : decl{ decl }, Expr{ Expr::DeclRefExprClass } {}
        ~DeclRefExpr() = default;    

        inline ValueDecl* GetValueDecl() { return decl; }

        static inline DeclRefExpr* Create(ASTContext& context, ValueDecl* decl, SourceRange range) {
            DeclRefExpr* instance = context.AllocateNode<DeclRefExpr>(decl);
            instance->SetResultType(decl->GetValueType());
            instance->SetSourceRange(range);
            return instance;
        }

    private:
        ValueDecl* decl;
    };
    

    class CastExpr : public Expr {
    public:
        CastExpr(Expr* expr) : expr{ expr }, Expr{ Expr::CastExprClass } {}
        ~CastExpr() = default;    

        inline Expr* GetExpr() { return expr; }

        static inline CastExpr* Create(ASTContext& context, Expr* expr, QualType type, SourceRange range) {
            CastExpr* instance = context.AllocateNode<CastExpr>(expr);
            instance->SetResultType(type);
            instance->SetSourceRange(range);
            return instance;
        }

    private:
        Expr* expr;
    };

    class BinaryArithmeticExpr : public Expr {
    public:
        BinaryArithmeticExpr(Expr* lhs, Expr* rhs, BinaryOperator::Kind op) :
            lhs{ lhs }, rhs{ rhs }, op{ op }, Expr{ Expr::BinaryArithmeticExprClass } {}
        ~BinaryArithmeticExpr() = default;

        inline Expr* GetLHS() { return lhs; }
        inline Expr* GetRHS() { return rhs; }
        inline BinaryOperator::Kind GetOperatorKind() const { return op; }

        static inline BinaryArithmeticExpr* Create(ASTContext& context, Expr* lhs, Expr* rhs, BinaryOperator& op) {
            BinaryArithmeticExpr* instance = context.AllocateNode<BinaryArithmeticExpr>(lhs, rhs, op.kind);
            instance->SetResultType(lhs->GetResultType());
            instance->SetSourceRange(SourceRange{ lhs->GetSourceRange().start, rhs->GetSourceRange().end });
            return instance;
        }

        static inline BinaryArithmeticExpr* Create(ASTContext& context, Expr* lhs, Expr* rhs, BinaryOperator::Kind op) {
            BinaryArithmeticExpr* instance = context.AllocateNode<BinaryArithmeticExpr>(lhs, rhs, op);
            instance->SetResultType(lhs->GetResultType());
            instance->SetSourceRange(SourceRange{ lhs->GetSourceRange().start, rhs->GetSourceRange().end });
            return instance;
        }

    private:
        Expr* lhs;
        Expr* rhs;
        BinaryOperator::Kind op;
    };

}