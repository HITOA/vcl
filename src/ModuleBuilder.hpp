#pragma once

#include <VCL/AST.hpp>
#include <VCL/ModuleUtils.hpp>

#include "Value.hpp"
#include "ModuleContext.hpp"

#include <llvm/ExecutionEngine/Orc/Core.h>


namespace VCL {

    /**
     * @brief Main class for building a JIT Module from an AST.
     *
     * This is a ASTVisitor. This should be used by calling the Accept() function on
     * the root node of the AST with a ptr to the ModuleBuilder as argument.
     */
    class ModuleBuilder : public ASTVisitor {
    public:
        ModuleBuilder() = delete;
        ModuleBuilder(ModuleContext* context);
        ~ModuleBuilder();

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

        inline void SetDISettings(ModuleDebugInformationSettings diSettings) { this->diSettings = diSettings; }

    public:
        ModuleContext* context;
        llvm::DIFile* file;
        ModuleDebugInformationSettings diSettings;
        Handle<Value> lastReturnedValue;
    };

}