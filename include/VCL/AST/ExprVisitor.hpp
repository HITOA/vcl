#pragma once

#include <VCL/AST/Expr.hpp>


namespace VCL {

    template<typename T>
    class ExprVisitor {
    public:
        inline T VisitExpr(Expr* expr) {
            switch (expr->GetExprClass())
            {
                case Expr::NumericLiteralExprClass: return VisitNumericLiteralExpr((NumericLiteralExpr*)expr);
                case Expr::LoadExprClass: return VisitLoadExpr((LoadExpr*)expr);
                case Expr::SplatExprClass: return VisitSplatExpr((SplatExpr*)expr);
                case Expr::DeclRefExprClass: return VisitDeclRefExpr((DeclRefExpr*)expr);
                case Expr::CastExprClass: return VisitCastExpr((CastExpr*)expr);
                case Expr::BinaryExprClass: return VisitBinaryExpr((BinaryExpr*)expr);
                case Expr::UnaryExprClass: return VisitUnaryExpr((UnaryExpr*)expr);
                case Expr::CallExprClass: return VisitCallExpr((CallExpr*)expr);
                case Expr::DependentCallExprClass: return VisitDependentCallExpr((DependentCallExpr*)expr);
                case Expr::FieldAccessExprClass: return VisitFieldAccessExpr((FieldAccessExpr*)expr);
                case Expr::DependentFieldAccessExprClass: return VisitDependentFieldAccessExpr((DependentFieldAccessExpr*)expr);
                case Expr::SubscriptExprClass: return VisitSubscriptExpr((SubscriptExpr*)expr);
                case Expr::NullExprClass: return VisitNullExpr((NullExpr*)expr);
                case Expr::AggregateExprClass: return VisitAggregateExpr((AggregateExpr*)expr);
                default: return T{};
            }
        }

    protected:
        virtual T VisitNumericLiteralExpr(NumericLiteralExpr* expr) = 0;
        virtual T VisitLoadExpr(LoadExpr* expr) = 0;
        virtual T VisitSplatExpr(SplatExpr* expr) = 0;
        virtual T VisitDeclRefExpr(DeclRefExpr* expr) = 0;
        virtual T VisitCastExpr(CastExpr* expr) = 0;
        virtual T VisitBinaryExpr(BinaryExpr* expr) = 0;
        virtual T VisitUnaryExpr(UnaryExpr* expr) = 0;
        virtual T VisitCallExpr(CallExpr* expr) = 0;
        virtual T VisitDependentCallExpr(DependentCallExpr* expr) = 0;
        virtual T VisitFieldAccessExpr(FieldAccessExpr* expr) = 0;
        virtual T VisitDependentFieldAccessExpr(DependentFieldAccessExpr* expr) = 0;
        virtual T VisitSubscriptExpr(SubscriptExpr* expr) = 0;
        virtual T VisitNullExpr(NullExpr* expr) = 0;
        virtual T VisitAggregateExpr(AggregateExpr* expr) = 0;
    };

}