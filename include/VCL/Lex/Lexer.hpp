#pragma once

#include <VCL/Core/SourceLocation.hpp>
#include <VCL/Lex/Token.hpp>

#include <llvm/Support/MemoryBufferRef.h>


namespace VCL {

    class Lexer {
    public:
        Lexer() = default;
        Lexer(const llvm::MemoryBufferRef& buffer);
        Lexer(const SourceRange& range);
        Lexer(const Lexer& lexer) = delete;
        Lexer(Lexer&& lexer) = delete;
        ~Lexer() = default;

        Lexer& operator=(const Lexer& lexer) = delete;
        Lexer& operator=(Lexer&& lexer) = delete;

        bool Lex(Token& token);

    private:
        SourceRange range{};
        SourceLocation currentLocation{};
    };

}