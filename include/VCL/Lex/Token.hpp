#pragma once

#include <VCL/Core/TokenKind.hpp>
#include <VCL/Core/SourceLocation.hpp>


namespace VCL {

    /**
     * Represent a Lexer's Token.
     */
    struct Token {
        /** The kind of token this is */
        TokenKind kind;
        /** The SourceRange corresponding to this Token */
        SourceRange range;
    };

}