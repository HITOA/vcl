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
            // Binary Bitwise
            BitwiseAnd, BitwiseOr, BitwiseXor,
            LeftShift, RightShift,
            // Binary Logical
            Greater, Lesser, GreaterEqual, LesserEqual,
            Equal, NotEqual, LogicalAnd, LogicalOr,
            // Binary Assignment
            Assignment, AssignmentAdd, AssignmentSub,
            AssignmentMul, AssignmentDiv, AssignmentRemainder,
            AssignmentBitwiseAnd, AssignmentBitwiseOr,
            AssignmentBitwiseXor, AssignmentLeftShift,
            AssignmentRightShift
        };

        Kind kind = None;
        OperatorAssociativity associativity = OperatorAssociativity::None;
        int precedence = -1;
    };

    enum class UnaryOperator {
        None,
        PrefixIncrement, PrefixDecrement,
        Plus, Minus, LogicalNot, BitwiseNot,
        PostfixIncrement, PostfixDecrement
    };

    inline BinaryOperator GetBinaryOperator(Token& token, bool onTemplateArgument) {
        switch (token.kind) {
            // Binary Arithmetic
            case TokenKind::Asterisk: return BinaryOperator{ BinaryOperator::Mul, OperatorAssociativity::Left, 12 };
            case TokenKind::Slash: return BinaryOperator{ BinaryOperator::Div, OperatorAssociativity::Left, 12 };
            case TokenKind::Percent: return BinaryOperator{ BinaryOperator::Remainder, OperatorAssociativity::Left, 12 };
            case TokenKind::Plus: return BinaryOperator{ BinaryOperator::Add, OperatorAssociativity::Left, 11 };
            case TokenKind::Minus: return BinaryOperator{ BinaryOperator::Sub, OperatorAssociativity::Left, 11 };
            // Binary Bitwise
            case TokenKind::GreaterGreater: return BinaryOperator{ BinaryOperator::RightShift, OperatorAssociativity::Left, 10 };
            case TokenKind::LesserLesser: return BinaryOperator{ BinaryOperator::LeftShift, OperatorAssociativity::Left, 10 };
            case TokenKind::Amp: return BinaryOperator{ BinaryOperator::BitwiseAnd, OperatorAssociativity::Left, 6 };
            case TokenKind::Caret: return BinaryOperator{ BinaryOperator::BitwiseXor, OperatorAssociativity::Left, 5 };
            case TokenKind::Pipe: return BinaryOperator{ BinaryOperator::BitwiseOr, OperatorAssociativity::Left, 4 };
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
            // Binary Assignment
            case TokenKind::Equal: return BinaryOperator{ BinaryOperator::Assignment, OperatorAssociativity::Right, 1 };
            case TokenKind::PlusEqual: return BinaryOperator{ BinaryOperator::AssignmentAdd, OperatorAssociativity::Right, 1 };
            case TokenKind::MinusEqual: return BinaryOperator{ BinaryOperator::AssignmentSub, OperatorAssociativity::Right, 1 };
            case TokenKind::AsteriskEqual: return BinaryOperator{ BinaryOperator::AssignmentMul, OperatorAssociativity::Right, 1 };
            case TokenKind::SlashEqual: return BinaryOperator{ BinaryOperator::AssignmentDiv, OperatorAssociativity::Right, 1 };
            case TokenKind::PercentEqual: return BinaryOperator{ BinaryOperator::AssignmentRemainder, OperatorAssociativity::Right, 1 };
            case TokenKind::AmpEqual: return BinaryOperator{ BinaryOperator::AssignmentBitwiseAnd, OperatorAssociativity::Right, 1 };
            case TokenKind::CaretEqual: return BinaryOperator{ BinaryOperator::AssignmentBitwiseXor, OperatorAssociativity::Right, 1 };
            case TokenKind::PipeEqual: return BinaryOperator{ BinaryOperator::AssignmentBitwiseOr, OperatorAssociativity::Right, 1 };
            case TokenKind::GreaterGreaterEqual: return BinaryOperator{ BinaryOperator::AssignmentRightShift, OperatorAssociativity::Right, 1 };
            case TokenKind::LesserLesserEqual: return BinaryOperator{ BinaryOperator::AssignmentLeftShift, OperatorAssociativity::Right, 1 };
            
            default: return BinaryOperator{};
        }
    }

}