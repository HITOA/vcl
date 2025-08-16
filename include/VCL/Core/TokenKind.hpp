#pragma once


namespace VCL {

    /**
     * Enum that represent all existing kind of VCL Token
     */
    enum class TokenKind : unsigned short {
        #define TOK(x) x,
        #include <VCL/Core/TokenKind.def>
        TokenKindMax
    };

}