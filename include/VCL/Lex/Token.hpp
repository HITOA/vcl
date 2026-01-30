#pragma once

#include <VCL/Core/TokenKind.hpp>
#include <VCL/Core/SourceLocation.hpp>
#include <VCL/Core/Identifier.hpp>


namespace VCL {

    /**
     * Represent a Lexer's Token.
     */
    struct Token {
        /** The kind of token this is */
        TokenKind kind;
        /** The SourceRange corresponding to this Token */
        SourceRange range;
        /** The IdentifierInfo of this token or nullptr if not an identifier */
        IdentifierInfo* identifier;
        /** For TokenKind::NumericConstant this indicate if the numeric value is floating point or not */
        bool isFloatingPoint;
    };

}