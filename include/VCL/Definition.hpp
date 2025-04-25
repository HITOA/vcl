#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#define FLAG(x) (1<<x)

#undef DEF
#define DEF(name, symbol, ...)

#undef TYPE_DEF
#define TYPE_DEF \
    DEF(Float,              "float") \
    DEF(Bool,               "bool") \
    DEF(Int,                "int") \
    DEF(Void,               "void") \
    DEF(VectorFloat,        "vfloat") \
    DEF(VectorBool,         "vbool") \
    DEF(VectorInt,          "vint")

#undef TYPE_QUALIFIER_DEF
#define TYPE_QUALIFIER_DEF \
    DEF(Const,              "const",    FLAG(0)) \
    DEF(Out,                "out",      FLAG(1)) \
    DEF(In,                 "in",       FLAG(2))

#undef KEYWORD_DEF
#define KEYWORD_DEF \
    DEF(Return,             "return") \
    DEF(If,                 "if") \
    DEF(Else,               "else") \
    DEF(While,              "while") \
    DEF(For,                "for") \
    DEF(Break,              "break") \
    DEF(Struct,             "struct")

#undef OPERATOR_DEF
#define OPERATOR_DEF \
    DEF(Dot,                ".") \
    DEF(Asterisk,           "*") \
    DEF(Slash,              "/") \
    DEF(Plus,               "+") \
    DEF(Minus,              "-") \
    DEF(Greater,            ">") \
    DEF(Less,               "<") \
    DEF(GreaterEqual,       ">=") \
    DEF(LessEqual,          "<=") \
    DEF(Equal,              "==") \
    DEF(NotEqual,           "!=") \
    DEF(LogicalAnd,         "&&") \
    DEF(LogicalOr,          "||") \
    DEF(Increment,          "++") \
    DEF(Decrement,          "--") \
    DEF(ExclmMark,          "!") \
    DEF(Assignment,         "=")

#undef PUNCTUATOR_DEF
#define PUNCTUATOR_DEF \
    DEF(Semicolon,          ";") \
    DEF(LPar,               "(") \
    DEF(RPar,               ")") \
    DEF(LBracket,           "{") \
    DEF(RBracket,           "}") \
    DEF(Coma,               ",") \
    DEF(LSquareBracket,     "[") \
    DEF(RSquareBracket,     "]")

namespace VCL {
#undef DEF
#define DEF(name, symbol, ...) name,
    enum class TokenType {
        Undefined,
        EndOfFile,
        Identifier,
        LiteralInt,
        LiteralFloat,

        TYPE_DEF
        TYPE_QUALIFIER_DEF
        KEYWORD_DEF
        OPERATOR_DEF
        PUNCTUATOR_DEF

        Max
    };

    struct Operator {
        enum class ID {
            None,
            // Binary Arithmetic
            Add, Sub, Mul, Div,
            // Binary Logical
            Greater, Less, GreaterEqual, LessEqual,
            Equal, NotEqual, LogicalAnd, LogicalOr,
            // Binary Assignment
            Assignment, AssignmentAdd, AssignmentSub,
            AssignmentMul, AssignmentDiv,
            // Prefix Arithmetic
            Plus, Minus, PreIncrement, PreDecrement,
            // Prefix Logical
            Not,
            // Postfix Arithmetic
            PostIncrement, PostDecrement,
            // Access
            FieldAccess
        } id = ID::None;

        enum class Kind {
            None,
            Arithmetic,
            Logical,
            Comparison,
            Assignment,
            FieldAccess
        } kind = Kind::None;

        enum class Associativity {
            None,
            Left,
            Right
        } associativity = Associativity::None;

        int precedence = -1;
    };

    struct TemplateArgument {
        union ValueUnion {
            int intValue;
            enum class TypeName {
                None,
                TYPE_DEF
            } typeValue = TypeName::None;
        } value;
        enum class TemplateValueType {
            Int,
            Typename
        } type = TemplateValueType::Typename;
    };

    /**
     * @brief The TypeInfo class contain all the needed information to debug and build a proper LLVM Type for JIT compilation.
     */
    struct TypeInfo {
#undef DEF
#define DEF(name, symbol, ...) name = __VA_ARGS__,
        enum class QualifierFlag {
            None = 0,
            TYPE_QUALIFIER_DEF
        } qualifiers = QualifierFlag::None;

#undef DEF
#define DEF(name, symbol, ...) name,
        enum class TypeName {
            None,
            Custom,
            Callable,
            TYPE_DEF
        } type = TypeName::None;

        std::string_view name = "";
        TemplateArgument arguments[8] = {};
        uint32_t templateArgsCount = 0;


        friend inline TypeInfo::QualifierFlag operator|(TypeInfo::QualifierFlag a, TypeInfo::QualifierFlag b) {
            return (TypeInfo::QualifierFlag)((int)a | (int)b);
        }

        friend inline TypeInfo::QualifierFlag operator|=(TypeInfo::QualifierFlag& a, TypeInfo::QualifierFlag b) {
            
            a = (TypeInfo::QualifierFlag)((int)a | (int)b);
            return a;
        }

        friend inline bool operator&(TypeInfo::QualifierFlag a, TypeInfo::QualifierFlag b) {
            return ((int)a & (int)b) != 0;
        }

        inline bool IsInput() const {
            return qualifiers & QualifierFlag::In;
        }

        inline bool IsOutput() const {
            return qualifiers & QualifierFlag::Out;
        }

        inline bool IsExtern() const {
            return IsInput() || IsOutput();
        }

        inline bool IsConst() const {
            return qualifiers & QualifierFlag::Const;
        }

        inline bool IsCallable() const {
            return type == TypeName::Callable;
        }
    };
}