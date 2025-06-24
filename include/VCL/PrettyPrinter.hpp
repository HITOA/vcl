#pragma once

#include <VCL/AST.hpp>


namespace VCL {

    class PrettyPrinter : public ASTVisitor {
    public:
        PrettyPrinter() = default;
        ~PrettyPrinter() = default;

        void VisitProgram(ASTProgram* node) override;
        void VisitCompoundStatement(ASTCompoundStatement* node) override;
        void VisitFunctionPrototype(ASTFunctionPrototype* node) override;
        void VisitFunctionDeclaration(ASTFunctionDeclaration* node) override;
        void VisitStructureDeclaration(ASTStructureDeclaration* node) override;
        void VisitTemplateDeclaration(ASTTemplateDeclaration* node) override;
        void VisitTemplateFunctionDeclaration(ASTTemplateFunctionDeclaration* node) override;
        void VisitReturnStatement(ASTReturnStatement* node) override;
        void VisitIfStatement(ASTIfStatement* node) override;
        void VisitWhileStatement(ASTWhileStatement* node) override;
        void VisitForStatement(ASTForStatement* node) override;
        void VisitBreakStatement(ASTBreakStatement* node) override;
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
        void VisitVariableExpression(ASTVariableExpression* node) override;
        void VisitVariableDeclaration(ASTVariableDeclaration* node) override;
        void VisitFunctionCall(ASTFunctionCall* node) override;
        void VisitAggregateExpression(ASTAggregateExpression* node) override;

        std::string GetBuffer();

    private:
        std::string buffer;
    };

}