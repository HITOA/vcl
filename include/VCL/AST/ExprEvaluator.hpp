#pragma once

#include <VCL/AST/ASTContext.hpp>
#include <VCL/AST/ConstantValue.hpp>
#include <VCL/AST/Expr.hpp>


namespace VCL {

    class ExprEvaluator {
    public:
        ExprEvaluator() = delete;
        ExprEvaluator(ASTContext& context) : context{ context } {}
        ExprEvaluator(const ExprEvaluator& other) = delete;
        ExprEvaluator(ExprEvaluator&& other) = delete;
        ~ExprEvaluator() = default;

        ExprEvaluator& operator=(const ExprEvaluator& other) = delete;
        ExprEvaluator& operator=(ExprEvaluator&& other) = delete;

        ConstantValue* VisitNumericLiteralExpr(NumericLiteralExpr* expr);
        ConstantValue* VisitDeclRefExpr(DeclRefExpr* expr);
        ConstantValue* VisitCastExpr(CastExpr* expr);
        ConstantValue* VisitSplatExpr(SplatExpr* expr);
        ConstantValue* VisitBinaryExpr(BinaryExpr* expr);
        ConstantValue* VisitUnaryExpr(UnaryExpr* expr);
        ConstantValue* VisitAggregateExpr(AggregateExpr* expr);
        ConstantValue* VisitNullExpr(NullExpr* expr);

        inline ConstantValue* Visit(Expr* expr) {
            switch (expr->GetExprClass()) {
                case Expr::NumericLiteralExprClass: return VisitNumericLiteralExpr((NumericLiteralExpr*)expr);
                case Expr::DeclRefExprClass: return VisitDeclRefExpr((DeclRefExpr*)expr);
                case Expr::CastExprClass: return VisitCastExpr((CastExpr*)expr);
                case Expr::SplatExprClass: return VisitSplatExpr((SplatExpr*)expr);
                case Expr::BinaryExprClass: return VisitBinaryExpr((BinaryExpr*)expr);
                case Expr::UnaryExprClass: return VisitUnaryExpr((UnaryExpr*)expr);
                case Expr::AggregateExprClass: return VisitAggregateExpr((AggregateExpr*)expr);
                case Expr::NullExprClass: return VisitNullExpr((NullExpr*)expr);
                default: return nullptr;
            }
        }

    private:
        ASTContext& context;
    };

}