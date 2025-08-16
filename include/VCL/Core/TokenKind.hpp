#pragma once


namespace VCL {

    enum TokenKind : unsigned short {
        #define TOK(x) x,
        #include <VCL/Core/TokenKind.def>
        #undef TOK
        TokenKindMax
    };

}