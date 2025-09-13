#include <VCL/AST/ExprEvaluator.hpp>



VCL::ConstantValue* VCL::ExprEvaluator::VisitNumericLiteralExpr(NumericLiteralExpr* expr) {
    return expr->GetValue();
}

VCL::ConstantValue* VCL::ExprEvaluator::VisitVariableRefExpr(VariableRefExpr* expr) {
    return nullptr;
}

VCL::ConstantValue* VCL::ExprEvaluator::VisitCastExpr(CastExpr* expr) {
    return nullptr;
}

VCL::ConstantValue* VCL::ExprEvaluator::VisitBinaryArithmeticExpr(BinaryArithmeticExpr* expr) {
    return nullptr;
}
