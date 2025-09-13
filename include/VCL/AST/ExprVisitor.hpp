#pragma once

#include <VCL/AST/Expr.hpp>
#include <VCL/AST/Visitor.hpp>


namespace VCL {

    template<typename T>
    class ExprVisitor {
    public:
        inline typename T::ReturnType Visit(Expr* expr) {
            switch (expr->GetExprClass()) {
                case Expr::NumericLiteralExprClass: return TryCallVisitNumericLiteralExpr((NumericLiteralExpr*)expr);
                case Expr::VariableRefExprClass: return TryCallVisitVariableRefExpr((VariableRefExpr*)expr);
                case Expr::CastExprClass: return TryCallVisitCastExpr((CastExpr*)expr);
                case Expr::BinaryArithmeticExprClass: return TryCallVisitBinaryArithmeticExpr((BinaryArithmeticExpr*)expr);
                default: return typename T::ReturnType{};
            }
        }

    private:
        DEFINE_TRY_CALL_VISIT_FUNC(VisitNumericLiteralExpr)
        DEFINE_TRY_CALL_VISIT_FUNC(VisitVariableRefExpr)
        DEFINE_TRY_CALL_VISIT_FUNC(VisitCastExpr)
        DEFINE_TRY_CALL_VISIT_FUNC(VisitBinaryArithmeticExpr)
    };

}