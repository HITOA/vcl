#include <VCL/Debug.hpp>


std::string VCL::ToString(Operator::ID id) {
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
        case Operator::ID::Plus: return "+ (unary)";
        case Operator::ID::Minus: return "- (unary)";
        case Operator::ID::PreIncrement: return "++ (prefix)";
        case Operator::ID::PreDecrement: return "-- (prefix)";

        // Prefix Logical
        case Operator::ID::Not: return "!";

        // Postfix Arithmetic
        case Operator::ID::PostIncrement: return "++ (postfix)";
        case Operator::ID::PostDecrement: return "-- (postfix)";

        // Access
        case Operator::ID::FieldAccess: return ".";

        default: return "<unknown operator>";
    }
}


std::string VCL::ToString(std::shared_ptr<TypeInfo> type) {
    switch (type->type) {
        case TypeInfo::TypeName::Custom: 
        {
            if (type->arguments.empty())
                return std::string{ type->name };
            std::string name{ type->name };
            for (auto argument : type->arguments)
                name += "_" + ToString(argument);
            return name;
        }
        case TypeInfo::TypeName::Callable: return "callable";
        case TypeInfo::TypeName::Float: return "float";
        case TypeInfo::TypeName::Bool: return "bool";
        case TypeInfo::TypeName::Int: return "int";
        case TypeInfo::TypeName::Void: return "void";
        case TypeInfo::TypeName::VectorFloat: return "vfloat";
        case TypeInfo::TypeName::VectorBool: return "vbool";
        case TypeInfo::TypeName::VectorInt: return "vint";
        case TypeInfo::TypeName::Array: {
            std::string name{ "array" };
            for (auto argument : type->arguments)
                name += "_" + ToString(argument);
            return name;
        }
        case TypeInfo::TypeName::Span: {
            std::string name{ "span" };
            for (auto argument : type->arguments)
                name += "_" + ToString(argument);
            return name;
        }
        default: return "unknown type";
    }
}

std::string VCL::ToString(std::shared_ptr<TemplateArgument> argument) {
    switch (argument->type)
    {
    case TemplateArgument::TemplateValueType::Int: return std::to_string(argument->intValue);
    case TemplateArgument::TemplateValueType::Typename: return ToString(argument->typeInfo);
        break;
    default: return "unknown type";
    }
}