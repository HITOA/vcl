#include <VCL/PrettyPrinter.hpp>



namespace VCL {
    static std::string ToString(Operator::ID id) {
        switch (id)
        {
            // Binary Arithmetic
            case Operator::ID::Add: return "+";
            case Operator::ID::Sub: return "-";
            case Operator::ID::Mul: return "*";
            case Operator::ID::Div: return "/";

            // Binary Logical / Comparison
            case Operator::ID::Greater: return ">";
            case Operator::ID::Less: return "<";
            case Operator::ID::GreaterEqual: return ">=";
            case Operator::ID::LessEqual: return "<=";
            case Operator::ID::Equal: return "==";
            case Operator::ID::NotEqual: return "!=";
            case Operator::ID::LogicalAnd: return "&&";
            case Operator::ID::LogicalOr: return "||";

            // Binary Assignment
            case Operator::ID::Assignment: return "=";
            case Operator::ID::AssignmentAdd: return "+=";
            case Operator::ID::AssignmentSub: return "-=";
            case Operator::ID::AssignmentMul: return "*=";
            case Operator::ID::AssignmentDiv: return "/=";

            // Prefix Arithmetic
            case Operator::ID::Plus: return "+";
            case Operator::ID::Minus: return "-";
            case Operator::ID::PreIncrement: return "++";
            case Operator::ID::PreDecrement: return "--";

            // Prefix Logical
            case Operator::ID::Not: return "!";

            // Postfix Arithmetic
            case Operator::ID::PostIncrement: return "++";
            case Operator::ID::PostDecrement: return "--";

            // Access
            case Operator::ID::FieldAccess: return ".";

            default: return "<unknown operator>";
        }
    }

    static std::string ToString(std::shared_ptr<TemplateArgument> argument);

    static std::string ToString(std::shared_ptr<TypeInfo> type) {
        std::string qualifiers = "";
        if (type->qualifiers & TypeInfo::QualifierFlag::Const)
            qualifiers += "const ";
        if (type->qualifiers & TypeInfo::QualifierFlag::In)
            qualifiers += "in ";
        if (type->qualifiers & TypeInfo::QualifierFlag::Out)
            qualifiers += "out ";
        switch (type->type) {
            case TypeInfo::TypeName::Custom: 
            {
                if (type->arguments.empty())
                    return std::string{ type->name };
                std::string name{ type->name };
                if (!type->arguments.empty()) {
                    name += "<";
                    bool b = false;
                    for (auto argument : type->arguments) {
                        if (b)
                            name += ", ";
                        name += ToString(argument);
                        b = true;
                    }
                    name += ">";
                }
                return qualifiers + name;
            }
            case TypeInfo::TypeName::Callable: return qualifiers + "callable";
            case TypeInfo::TypeName::Float: return qualifiers + "float";
            case TypeInfo::TypeName::Bool: return qualifiers + "bool";
            case TypeInfo::TypeName::Int: return qualifiers + "int";
            case TypeInfo::TypeName::Void: return qualifiers + "void";
            case TypeInfo::TypeName::VectorFloat: return qualifiers + "vfloat";
            case TypeInfo::TypeName::VectorBool: return qualifiers + "vbool";
            case TypeInfo::TypeName::VectorInt: return qualifiers + "vint";
            case TypeInfo::TypeName::Array: {
                std::string name{ "array<" };
                bool b = false;
                for (auto argument : type->arguments) {
                    if (b)
                        name += ", ";
                    name += ToString(argument);
                    b = true;
                }
                return qualifiers + name + ">";
            }
            case TypeInfo::TypeName::Span: {
                std::string name{ "span<" };
                bool b = false;
                for (auto argument : type->arguments) {
                    if (b)
                        name += ", ";
                    name += ToString(argument);
                    b = true;
                }
                return qualifiers + name + ">";
            }
            default: return "unknown type";
        }
    }

    static std::string ToString(std::shared_ptr<TemplateArgument> argument) {
        switch (argument->type)
        {
        case TemplateArgument::TemplateValueType::Int: return std::to_string(argument->intValue);
        case TemplateArgument::TemplateValueType::Typename: return ToString(argument->typeInfo);
            break;
        default: return "unknown type";
        }
    }

    static std::string Indent(const std::string& str) {
        std::string newstr = "\t";
        for (auto& c : str) {
            newstr += c;
            if (c == '\n')
                newstr += "\t";
        }
        return newstr;
    }
}

void VCL::PrettyPrinter::VisitProgram(ASTProgram* node) {
    std::string program = "";
    for (auto& statement : node->statements) {
        statement->Accept(this);
        // Hacky but.. heh
        if (!buffer.empty()) {
            if (buffer[buffer.size() - 1] == '}')
                program += buffer + "\n";
            else
                program += buffer + ";\n";
        }
    }
    buffer = program;
}

void VCL::PrettyPrinter::VisitCompoundStatement(ASTCompoundStatement* node) {
    std::string compound = "{\n";
    for (auto& statement : node->statements) {
        statement->Accept(this);
        compound += Indent(buffer) + ";\n";
    }
    compound += "}";
    buffer = compound;
}

void VCL::PrettyPrinter::VisitFunctionPrototype(ASTFunctionPrototype* node) {
    std::string functionPrototype = ToString(node->type) + " " + node->name + "(";
    bool b = false;
    for (auto& arg : node->arguments) {
        if (b)
            functionPrototype += ", ";
        functionPrototype += ToString(arg->type) + " " + arg->name;
        b = true;
    }
    functionPrototype += ")";
    buffer = functionPrototype;
}

void VCL::PrettyPrinter::VisitFunctionDeclaration(ASTFunctionDeclaration* node) {
    node->prototype->Accept(this);
    std::string function = buffer + " ";
    node->body->Accept(this);
    function += buffer;
    buffer = function;
}

void VCL::PrettyPrinter::VisitStructureDeclaration(ASTStructureDeclaration* node) {
    std::string r = "struct " + node->name + " {\n";
    for (auto& field : node->fields)
        r += Indent(ToString(field->type) + " " + field->name + ";\n");
    r += "}";
    buffer = r;
}

void VCL::PrettyPrinter::VisitTemplateDeclaration(ASTTemplateDeclaration* node) {
    std::string r = "struct " + node->name + "<";
    bool b = false;
    for (auto& templateArg : node->parameters) {
        if (b)
            r += ", ";
        switch (templateArg->type) {
            case TemplateArgument::TemplateValueType::Int:
                r += "int ";
            case TemplateArgument::TemplateValueType::Typename:
                r += "typename ";
        }
        r += templateArg->name;
        b = true;
    }
    r += "> {\n";
    for (auto& field : node->fields)
        r += Indent(ToString(field->type) + " " + field->name + ";\n");
    r += "}";
    buffer = r;
}

void VCL::PrettyPrinter::VisitTemplateFunctionDeclaration(ASTTemplateFunctionDeclaration* node) {
    std::string r = ToString(node->type) + " " + node->name + "<";
    bool b = false;
    for (auto& templateArg : node->parameters) {
        if (b)
            r += ", ";
        switch (templateArg->type) {
            case TemplateArgument::TemplateValueType::Int:
                r += "int ";
            case TemplateArgument::TemplateValueType::Typename:
                r += "typename ";
        }
        r += templateArg->name;
        b = true;
    }
    r += ">(";
    b = false;
    for (auto& arg : node->arguments) {
        if (b)
            r += ", ";
        r += ToString(arg->type) + " " + arg->name;
        b = true;
    }
    r += ") ";
    node->body->Accept(this);
    r += buffer;
    buffer = r;
}

void VCL::PrettyPrinter::VisitReturnStatement(ASTReturnStatement* node) {
    node->expression->Accept(this);
    buffer = "return " + buffer;
}

void VCL::PrettyPrinter::VisitIfStatement(ASTIfStatement* node) {
    std::string result = "if (";
    node->condition->Accept(this);
    result += buffer + ")\n";
    node->thenStmt->Accept(this);
    result += buffer;

    if (node->elseStmt) {
        node->elseStmt->Accept(this);
        result += " else " + buffer;
    }

    buffer = result;
}

void VCL::PrettyPrinter::VisitWhileStatement(ASTWhileStatement* node) {
    std::string result = "while (";
    node->condition->Accept(this);
    result += buffer + ")\n";
    node->thenStmt->Accept(this);
    result += buffer;
    buffer = result;
}

void VCL::PrettyPrinter::VisitForStatement(ASTForStatement* node) {
    std::string result = "for (";
    node->start->Accept(this);
    result += buffer + "; ";
    node->condition->Accept(this);
    result += buffer + "; ";
    node->end->Accept(this);
    result += buffer + ")\n";
    node->thenStmt->Accept(this);
    if (buffer[buffer.size() - 1] != '}')
        buffer = Indent(buffer);
    result += buffer;
    buffer = result;
}

void VCL::PrettyPrinter::VisitBreakStatement(ASTBreakStatement* node) {
    buffer = "break";
}

void VCL::PrettyPrinter::VisitBinaryArithmeticExpression(ASTBinaryArithmeticExpression* node) {
    // I don't want to deal with precedence for now so i'll just put parenthesis
    std::string result = "(";
    node->lhs->Accept(this);
    result += buffer + " " + ToString(node->op) + " ";
    node->rhs->Accept(this);
    result += buffer + ")";
    buffer = result;
}

void VCL::PrettyPrinter::VisitBinaryLogicalExpression(ASTBinaryLogicalExpression* node) {
    // I don't want to deal with precedence for now so i'll just put parenthesis
    std::string result = "(";
    node->lhs->Accept(this);
    result += buffer + " " + ToString(node->op) + " ";
    node->rhs->Accept(this);
    result += buffer + ")";
    buffer = result;
}

void VCL::PrettyPrinter::VisitBinaryComparisonExpression(ASTBinaryComparisonExpression* node) {
    // I don't want to deal with precedence for now so i'll just put parenthesis
    std::string result = "(";
    node->lhs->Accept(this);
    result += buffer + " " + ToString(node->op) + " ";
    node->rhs->Accept(this);
    result += buffer + ")";
    buffer = result;
}

void VCL::PrettyPrinter::VisitAssignmentExpression(ASTAssignmentExpression* node) {
    std::string result = "";
    node->lhs->Accept(this);
    result += buffer + " " + ToString(node->op) + " ";
    node->rhs->Accept(this);
    result += buffer;
    buffer = result;
}

void VCL::PrettyPrinter::VisitPrefixArithmeticExpression(ASTPrefixArithmeticExpression* node) {
    node->expression->Accept(this);
    buffer = ToString(node->op) + buffer;
}

void VCL::PrettyPrinter::VisitPrefixLogicalExpression(ASTPrefixLogicalExpression* node) {
    node->expression->Accept(this);
    buffer = ToString(node->op) + buffer;
}

void VCL::PrettyPrinter::VisitPostfixArithmeticExpression(ASTPostfixArithmeticExpression* node) {
    node->expression->Accept(this);
    buffer = buffer + ToString(node->op);
}

void VCL::PrettyPrinter::VisitFieldAccessExpression(ASTFieldAccessExpression* node) {
    node->expression->Accept(this);
    buffer = buffer + "." + node->fieldName;
}

void VCL::PrettyPrinter::VisitSubscriptExpression(ASTSubscriptExpression* node) {
    node->expression->Accept(this);
    std::string result = buffer + "[";
    node->index->Accept(this);
    buffer = result + buffer + "]";
}

void VCL::PrettyPrinter::VisitLiteralExpression(ASTLiteralExpression* node) {
    switch (node->type) {
        case TypeInfo::TypeName::Float:
            buffer = std::to_string(node->fValue);
            break;
        case TypeInfo::TypeName::Int:
            buffer = std::to_string(node->iValue);
            break;
    }
}

void VCL::PrettyPrinter::VisitIdentifierExpression(ASTIdentifierExpression* node) {
    buffer = node->name;
}

void VCL::PrettyPrinter::VisitVariableDeclaration(ASTVariableDeclaration* node) {
    std::string result = ToString(node->type) + " " + node->name;
    if (node->expression) {
        node->expression->Accept(this);
        result += " = " + buffer;
    }
    buffer = result;
}

void VCL::PrettyPrinter::VisitFunctionCall(ASTFunctionCall* node) {
    std::string result = node->name;
    if (!node->templateArguments.empty()) {
        result += "<";
        bool b = false;
        for (auto& templateArg : node->templateArguments) {
            if (b)
                result += ", ";
            result += ToString(templateArg);
            b = true;
        }
        result += ">";
    }
    result += "(";
    bool b = false;
    for (auto& arg : node->arguments) {
        if (b)
            result += ", ";
        arg->Accept(this);
        result += buffer;
        b = true;
    }
    result += ")";
    buffer = result;
}

void VCL::PrettyPrinter::VisitAggregateExpression(ASTAggregateExpression* node) {
    std::string r = "{ ";
    bool b = false;
    for (auto& expr : node->values) {
        if (b)
            r += ", ";
        expr->Accept(this);
        r += buffer;
        b = true;
    }
    r += " }";
    buffer = r;
}

void VCL::PrettyPrinter::VisitDirective(ASTDirective* node) {
    buffer = "";
}

std::string VCL::PrettyPrinter::GetBuffer() {
    return buffer;
}