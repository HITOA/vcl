#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <memory>
#include <vector>

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
    DEF(VectorInt,          "vint") \
    DEF(Array,              "array") \
    DEF(Span,               "span")

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
    DEF(Struct,             "struct") \
    DEF(Typename,           "typename")

#undef OPERATOR_DEF
#define OPERATOR_DEF \
    DEF(Dot,                ".") \
    DEF(Asterisk,           "*") \
    DEF(Slash,              "/") \
    DEF(Remainder,          "%") \
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
        LiteralString,

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
            Add, Sub, Mul, Div, Remainder,
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
            FieldAccess, Subscript
        } id = ID::None;

        enum class Kind {
            None,
            Arithmetic,
            Logical,
            Comparison,
            Assignment,
            FieldAccess,
            Subscript
        } kind = Kind::None;

        enum class Associativity {
            None,
            Left,
            Right
        } associativity = Associativity::None;

        int precedence = -1;
    };

    class TypeInfo;

    struct TemplateArgument {
        int intValue;
        std::shared_ptr<TypeInfo> typeInfo;
        enum class TemplateValueType {
            Int,
            Typename
        } type = TemplateValueType::Typename;
    };

    struct RuntimeTypeInfo {
        size_t sizeInBytes = 0;
        size_t alignmentInBytes = 0;
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
            Aggregate,
            TYPE_DEF
        } type = TypeName::None;

        std::string_view name = "";
        std::vector<std::shared_ptr<TemplateArgument>> arguments{};

        RuntimeTypeInfo rtInfo{};

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

        inline bool IsVector() const {
            return type == TypeName::VectorFloat || type == TypeName::VectorBool || type == TypeName::VectorInt;
        }

        inline bool IsGivenByValue() const {
            return (!IsOutput() && type != TypeName::Custom && type != TypeName::Array) || IsInput();
        }

        inline bool IsGivenByReference() const {
            return !IsGivenByValue();
        }
    };
}