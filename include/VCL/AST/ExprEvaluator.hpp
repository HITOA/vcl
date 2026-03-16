#pragma once

#include <VCL/AST/ASTContext.hpp>
#include <VCL/AST/ConstantValue.hpp>
#include <VCL/AST/ExprVisitor.hpp>


namespace VCL {

    class ExprEvaluator : public ExprVisitor<ConstantValue*> {
    public:
        ExprEvaluator() = delete;
        ExprEvaluator(ASTContext& context) : context{ context } {}
        ExprEvaluator(const ExprEvaluator& other) = delete;
        ExprEvaluator(ExprEvaluator&& other) = delete;
        ~ExprEvaluator() = default;

        ExprEvaluator& operator=(const ExprEvaluator& other) = delete;
        ExprEvaluator& operator=(ExprEvaluator&& other) = delete;

        inline ConstantValue* Visit(Expr* expr) {
            if (expr->GetConstantValue() != nullptr)
                return expr->GetConstantValue();
            return VisitExpr(expr);
        }

        ConstantValue* VisitNumericLiteralExpr(NumericLiteralExpr* expr) override;
        ConstantValue* VisitDeclRefExpr(DeclRefExpr* expr) override;
        ConstantValue* VisitCastExpr(CastExpr* expr) override;
        ConstantValue* VisitSplatExpr(SplatExpr* expr) override;
        ConstantValue* VisitBinaryExpr(BinaryExpr* expr) override;
        ConstantValue* VisitUnaryExpr(UnaryExpr* expr) override;
        ConstantValue* VisitAggregateExpr(AggregateExpr* expr) override;
        ConstantValue* VisitNullExpr(NullExpr* expr) override;
        ConstantValue* VisitLoadExpr(LoadExpr* expr) override;
        ConstantValue* VisitCallExpr(CallExpr* expr) override;
        ConstantValue* VisitDependentCallExpr(DependentCallExpr* expr) override;
        ConstantValue* VisitFieldAccessExpr(FieldAccessExpr* expr) override;
        ConstantValue* VisitDependentFieldAccessExpr(DependentFieldAccessExpr* expr) override;
        ConstantValue* VisitSubscriptExpr(SubscriptExpr* expr) override;

    private:
        ASTContext& context;
    };

}