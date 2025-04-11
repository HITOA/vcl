#pragma once

#include <cstddef>
#include <cstdint>

#define FLAG(x) (1<<x)

#undef DEF
#define DEF(name, symbol, ...)

#undef TYPE_DEF
#define TYPE_DEF \
    DEF(FLOAT,              "float") \
    DEF(BOOLEAN,            "bool") \
    DEF(INT,                "int") \
    DEF(VOID,               "void") \
    DEF(VFLOAT,             "vfloat")

#undef TYPE_QUALIFIER_DEF
#define TYPE_QUALIFIER_DEF \
    DEF(CONST,              "const",    FLAG(0)) \
    DEF(OUT,                "out",      FLAG(1)) \
    DEF(IN,                 "in",       FLAG(2))

#undef TYPE_COMPOSITE_DEF
#define TYPE_COMPOSITE_DEF \
    DEF(BUFFER,             "ring") \
    DEF(ARRAY,              "array")

#undef KEYWORD_DEF
#define KEYWORD_DEF \
    DEF(RETURN,             "return") \
    DEF(IF,                 "if") \
    DEF(ELSE,               "else") \
    DEF(WHILE,              "while") \
    DEF(FOR,                "for")

#undef UNARY_OPERATOR_DEF
#define UNARY_OPERATOR_DEF \
    DEF(PLUS,               "+",        2) \
    DEF(MINUS,              "-",        2) \
    DEF(NOT,                "!",        2)

#undef BINARY_OPERATOR_DEF
#define BINARY_OPERATOR_DEF \
    DEF(MULTIPLICATION,     "*",        3) \
    DEF(DIVISION,           "/",        3) \
    DEF(ADDITION,           "+",        4) \
    DEF(SUBSTRACTION,       "-",        4) \
    DEF(SUPERIOR,           ">",        6) \
    DEF(INFERIOR,           "<",        6) \
    DEF(SUPERIOREQUAL,      ">=",       6) \
    DEF(INFERIOREQUAL,      "<=",       6) \
    DEF(EQUAL,              "==",       7) \
    DEF(NOTEQUAL,           "!=",       7) \
    DEF(LOGICALAND,         "&&",       11) \
    DEF(LOGICALOR,          "||",       12) \
    DEF(ASSIGNMENT,         "=",        14)

#undef PUNCTUATOR_DEF
#define PUNCTUATOR_DEF \
    DEF(SEMICOLON,          ";") \
    DEF(LPAR,               "(") \
    DEF(RPAR,               ")") \
    DEF(LBRACKET,           "{") \
    DEF(RBRACKET,           "}") \
    DEF(COMA,               ",") \
    DEF(LSQUAREBRACKET,     "[") \
    DEF(RSQUAREBRACKER,     "]")

namespace VCL {
#undef DEF
#define DEF(name, symbol, ...) name,
    enum class UnaryOpType {
        UNARY_OPERATOR_DEF
    };

    enum class BinaryOpType {
        BINARY_OPERATOR_DEF
    };


    /**
     * @brief The TypeInfo class contain all the needed information to debug and build a proper LLVM Type for JIT compilation.
     */
    struct TypeInfo {
#undef DEF
#define DEF(name, symbol, ...) name,
        enum class CompositeType {
            NONE = 0,
            TYPE_COMPOSITE_DEF
        } compositeType = CompositeType::NONE;

#undef DEF
#define DEF(name, symbol, ...) name = __VA_ARGS__,
        enum class QualifierFlag {
            NONE = 0,
            TYPE_QUALIFIER_DEF
        } qualifiers = QualifierFlag::NONE;

#undef DEF
#define DEF(name, symbol, ...) name,
        enum class TypeName {
            NONE,
            CALLABLE,
            TYPE_DEF
        } type = TypeName::NONE;

        size_t bufferSize = 0;


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
            return qualifiers & QualifierFlag::IN;
        }

        inline bool IsOutpout() const {
            return qualifiers & QualifierFlag::OUT;
        }

        inline bool IsExtern() const {
            return IsInput() || IsOutpout();
        }

        inline bool IsConst() const {
            return qualifiers & QualifierFlag::CONST;
        }

        inline bool IsCallable() const {
            return type == TypeName::CALLABLE;
        }
    };

}