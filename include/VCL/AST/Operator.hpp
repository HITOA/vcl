#pragma once

#include <VCL/Lex/Token.hpp>


namespace VCL {

    enum class OperatorAssociativity {
        None,
        Left,
        Right
    };

    struct BinaryOperator {
        enum Kind {
            None,
            // Binary Arithmetic
            Add, Sub, Mul, Div, Remainder,
            // Binary Logical
            Greater, Lesser, GreaterEqual, LesserEqual,
            Equal, NotEqual, LogicalAnd, LogicalOr,
            // Binary Assignment
            Assignment, AssignmentAdd, AssignmentSub,
            AssignmentMul, AssignmentDiv,
        };

        Kind kind = None;
        OperatorAssociativity associativity = OperatorAssociativity::None;
        int precedence = -1;
    };

    inline BinaryOperator GetBinaryOperator(Token& token, bool onTemplateArgument) {
        switch (token.kind) {
            // Binary Arithmetic
            case TokenKind::Asterisk: return BinaryOperator{ BinaryOperator::Mul, OperatorAssociativity::Left, 12 };
            case TokenKind::Slash: return BinaryOperator{ BinaryOperator::Div, OperatorAssociativity::Left, 12 };
            case TokenKind::Percent: return BinaryOperator{ BinaryOperator::Remainder, OperatorAssociativity::Left, 12 };
            case TokenKind::Plus: return BinaryOperator{ BinaryOperator::Add, OperatorAssociativity::Left, 11 };
            case TokenKind::Minus: return BinaryOperator{ BinaryOperator::Sub, OperatorAssociativity::Left, 11 };
            // Binary Logical
            case TokenKind::Greater: {
                if (onTemplateArgument)
                    return BinaryOperator{};
                return BinaryOperator{ BinaryOperator::Greater, OperatorAssociativity::Left, 9 };
            }
            case TokenKind::Lesser: return BinaryOperator{ BinaryOperator::Lesser, OperatorAssociativity::Left, 9 };
            case TokenKind::GreaterEqual: return BinaryOperator{ BinaryOperator::GreaterEqual, OperatorAssociativity::Left, 9 };
            case TokenKind::LesserEqual: return BinaryOperator{ BinaryOperator::LesserEqual, OperatorAssociativity::Left, 9 };
            case TokenKind::EqualEqual: return BinaryOperator{ BinaryOperator::Equal, OperatorAssociativity::Left, 8 };
            case TokenKind::ExclaimEqual: return BinaryOperator{ BinaryOperator::NotEqual, OperatorAssociativity::Left, 8 };
            case TokenKind::AmpAmp: return BinaryOperator{ BinaryOperator::LogicalAnd, OperatorAssociativity::Left, 4 };
            case TokenKind::PipePipe: return BinaryOperator{ BinaryOperator::LogicalOr, OperatorAssociativity::Left, 3 };
            case TokenKind::Equal: return BinaryOperator{ BinaryOperator::Assignment, OperatorAssociativity::Right, 1 };
            case TokenKind::PlusEqual: return BinaryOperator{ BinaryOperator::AssignmentAdd, OperatorAssociativity::Right, 1 };
            case TokenKind::MinusEqual: return BinaryOperator{ BinaryOperator::AssignmentSub, OperatorAssociativity::Right, 1 };
            case TokenKind::AsteriskEqual: return BinaryOperator{ BinaryOperator::AssignmentMul, OperatorAssociativity::Right, 1 };
            case TokenKind::SlashEqual: return BinaryOperator{ BinaryOperator::AssignmentDiv, OperatorAssociativity::Right, 1 };
            
            default: return BinaryOperator{};
        }
    }

}