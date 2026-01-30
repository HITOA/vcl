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

    /**
     * Get the name of the given enum value as a string
     */
    inline const char* ToString(TokenKind kind) {
        switch (kind) {
            #define TOK(x) case TokenKind::x: return #x;
            #include <VCL/Core/TokenKind.def>
            default: return "Unknown";
        }
    }

}