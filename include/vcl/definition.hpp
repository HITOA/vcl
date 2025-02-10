#pragma once

#include <cstddef>
#include <cstdint>

#define FLAG(x) (1<<x)

#undef DEF
#define DEF(name, symbol, ...)

#undef TYPE_DEF
#define TYPE_DEF \
    DEF(FLOAT,              "float") \
    DEF(VOID,               "void") \
    DEF(VFLOAT,             "vfloat")

#undef TYPE_QUALIFIER_DEF
#define TYPE_QUALIFIER_DEF \
    DEF(BUFFER,             "ring",     FLAG(0)) \
    DEF(ARRAY,              "array",    FLAG(1)) \
    DEF(CONST,              "const",    FLAG(2)) \
    DEF(OUT,                "out",      FLAG(3)) \
    DEF(IN,                 "in",       FLAG(4))

#undef KEYWORD_DEF
#define KEYWORD_DEF \
    DEF(RETURN,             "return")

#undef UNARY_OPERATOR_DEF
#define UNARY_OPERATOR_DEF \
    DEF(PLUS,               "+",        2) \
    DEF(MINUS,              "-",        2) 

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
    DEF(ASSIGNMENT,         "=",        14)

#undef PUNCTUATOR_DEF
#define PUNCTUATOR_DEF \
    DEF(SEMICOLON,          ";") \
    DEF(LPAR,               "(") \
    DEF(RPAR,               ")") \
    DEF(LBRACKET,           "{") \
    DEF(RBRACKET,           "}") \
    DEF(COMA,               ",")
