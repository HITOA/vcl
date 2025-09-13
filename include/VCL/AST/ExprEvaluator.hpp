#pragma once

#include <VCL/AST/ConstantValue.hpp>
#include <VCL/AST/Expr.hpp>


namespace VCL {

    class ExprEvaluator {
    public:
        ConstantValue* VisitNumericLiteralExpr(NumericLiteralExpr* expr);
        ConstantValue* VisitVariableRefExpr(VariableRefExpr* expr);
        ConstantValue* VisitCastExpr(CastExpr* expr);
        ConstantValue* VisitBinaryArithmeticExpr(BinaryArithmeticExpr* expr);
    };

}