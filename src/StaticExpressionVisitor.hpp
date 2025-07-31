#pragma once

#include <VCL/AST.hpp>
#include <VCL/Module.hpp>
#include <VCL/Meta.hpp>


namespace VCL {

    class StaticExpressionVisitor : public ASTVisitor {
    public:
        bool Evaluate(ASTExpression* expression);

        void VisitBinaryArithmeticExpression(ASTBinaryArithmeticExpression* node) override;
        void VisitBinaryLogicalExpression(ASTBinaryLogicalExpression* node) override;
        void VisitBinaryComparisonExpression(ASTBinaryComparisonExpression* node) override;
        void VisitAssignmentExpression(ASTAssignmentExpression* node) override;
        void VisitPrefixArithmeticExpression(ASTPrefixArithmeticExpression* node) override;
        void VisitPrefixLogicalExpression(ASTPrefixLogicalExpression* node) override;
        void VisitPostfixArithmeticExpression(ASTPostfixArithmeticExpression* node) override;
        void VisitFieldAccessExpression(ASTFieldAccessExpression* node) override;
        void VisitSubscriptExpression(ASTSubscriptExpression* node) override;
        void VisitLiteralExpression(ASTLiteralExpression* node) override;
        void VisitIdentifierExpression(ASTIdentifierExpression* node) override;
        void VisitVariableDeclaration(ASTVariableDeclaration* node) override;
        void VisitFunctionCall(ASTFunctionCall* node) override;
        void VisitAggregateExpression(ASTAggregateExpression* node) override;

    public:
        union {
            int lastIntValue;
            float lastFloatValue;
            bool lastFlagValue;
        };
        enum class LastValueType {
            None,
            Int,
            Float,
            Flag
        } lastValueType = LastValueType::None;
        bool lastValueDefined = false;

        std::shared_ptr<VCL::MetaState> state = nullptr;
    };

}