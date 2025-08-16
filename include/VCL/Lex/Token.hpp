#pragma once

#include <VCL/Core/TokenKind.hpp>
#include <VCL/Core/SourceLocation.hpp>


namespace VCL {

    struct Token {
        TokenKind kind;
        SourceRange range;
    };

}