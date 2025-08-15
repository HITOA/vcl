#include "StaticExpressionVisitor.hpp"

#include <VCL/Error.hpp>
#include <VCL/Directive.hpp>


bool VCL::StaticExpressionVisitor::Evaluate(ASTExpression* expression) {
    expression->Accept(this);
    switch (lastValueType) {
        case LastValueType::Int:
            return lastIntValue != 0;
        case LastValueType::Float:
            return lastFloatValue != 0;
        case LastValueType::Flag:
            return lastFlagValue;
        default:
            return false;
    }
}

void VCL::StaticExpressionVisitor::VisitBinaryArithmeticExpression(ASTBinaryArithmeticExpression* node) {

}

void VCL::StaticExpressionVisitor::VisitBinaryLogicalExpression(ASTBinaryLogicalExpression* node) {

}

void VCL::StaticExpressionVisitor::VisitBinaryComparisonExpression(ASTBinaryComparisonExpression* node) {

}

void VCL::StaticExpressionVisitor::VisitAssignmentExpression(ASTAssignmentExpression* node) {
    throw new Exception{ "Assignment operator cannot be used in a static expression", node->location };
}

void VCL::StaticExpressionVisitor::VisitPrefixArithmeticExpression(ASTPrefixArithmeticExpression* node) {

}

void VCL::StaticExpressionVisitor::VisitPrefixLogicalExpression(ASTPrefixLogicalExpression* node) {
    node->expression->Accept(this);
    switch (node->op) {
        case Operator::ID::Not:
            lastValueDefined = !lastValueDefined;
            if (lastValueType == LastValueType::Flag)   
                lastFlagValue = !lastFlagValue;
            break;
    }
}

void VCL::StaticExpressionVisitor::VisitPostfixArithmeticExpression(ASTPostfixArithmeticExpression* node) {

}

void VCL::StaticExpressionVisitor::VisitFieldAccessExpression(ASTFieldAccessExpression* node) {
    throw new Exception{ "Field access operator cannot be used in a static expression", node->location };
}

void VCL::StaticExpressionVisitor::VisitSubscriptExpression(ASTSubscriptExpression* node) {
    throw new Exception{ "Subscript operator cannot be used in a static expression", node->location };
}

void VCL::StaticExpressionVisitor::VisitLiteralExpression(ASTLiteralExpression* node) {
    switch (node->type) {
        case TypeInfo::TypeName::Int:
            lastIntValue = node->iValue;
            lastValueType = LastValueType::Int;
            break;
        case TypeInfo::TypeName::Float:
            lastFloatValue = node->fValue;
            lastValueType = LastValueType::Float;
        default:
            throw new Exception{ "Unsupported literal expression in static expression", node->location };
    }
    lastValueDefined = false;
}

void VCL::StaticExpressionVisitor::VisitIdentifierExpression(ASTIdentifierExpression* node) {
    std::shared_ptr<DefineDirective::DefineDirectiveMetaComponent> component = state->GetOrCreate<DefineDirective::DefineDirectiveMetaComponent>();
    if (component->Defined(node->name)) {
        ASTLiteralExpression* expression = component->GetDefine(node->name);
        if (expression) {
            expression->Accept(this);
        }
        else {
            lastFlagValue = true;
            lastValueType = LastValueType::Flag;
        }
        lastValueDefined = true;
    } else {
        lastValueType = LastValueType::None;
        lastValueDefined = false;
    }
}

void VCL::StaticExpressionVisitor::VisitVariableDeclaration(ASTVariableDeclaration* node) {
    throw new Exception{ "Variable declaration cannot be made in a static expression", node->location };
}

void VCL::StaticExpressionVisitor::VisitFunctionCall(ASTFunctionCall* node) {
    if (node->name == "defined") {
        if (node->arguments.size() != 1)
            throw new Exception{ "The `defined` function only take on argument.", node->location };
        if (node->templateArguments.size() != 0)
            throw new Exception{ "Templated function isn't supported inside static expression", node->location };
        node->arguments[0]->Accept(this);
        lastFlagValue = lastValueDefined;
        lastValueType = LastValueType::Flag;
        lastValueDefined = false;
    } else {
        throw new Exception{ "Unsupported function call given in a static expression", node->location };
    }
}

void VCL::StaticExpressionVisitor::VisitAggregateExpression(ASTAggregateExpression* node) {
    throw new Exception{ "Aggregate values cannot be used in a static expression", node->location };
}