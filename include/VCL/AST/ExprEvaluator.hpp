#pragma once

#include <VCL/AST/ConstantValue.hpp>
#include <VCL/AST/Expr.hpp>


namespace VCL {

    class ExprEvaluator {
    public:
        ConstantValue* VisitNumericLiteralExpr(NumericLiteralExpr* expr);
        ConstantValue* VisitDeclRefExpr(DeclRefExpr* expr);
        ConstantValue* VisitCastExpr(CastExpr* expr);
        ConstantValue* VisitBinaryArithmeticExpr(BinaryArithmeticExpr* expr);

        inline ConstantValue* Visit(Expr* expr) {
            switch (expr->GetExprClass()) {
                case Expr::NumericLiteralExprClass: return VisitNumericLiteralExpr((NumericLiteralExpr*)expr);
                case Expr::DeclRefExprClass: return VisitDeclRefExpr((DeclRefExpr*)expr);
                case Expr::CastExprClass: return VisitCastExpr((CastExpr*)expr);
                case Expr::BinaryArithmeticExprClass: return VisitBinaryArithmeticExpr((BinaryArithmeticExpr*)expr);
                default: return nullptr;
            }
        }
    };

}