#pragma once

#include <VCL/Core/SourceLocation.hpp>
#include <VCL/Core/Diagnostic.hpp>
#include <VCL/Lex/Token.hpp>

#include <llvm/Support/MemoryBufferRef.h>
#include <llvm/ADT/StringMap.h>


namespace VCL {

    /**
     * Provide the most simple interface to tokenize a given Source without support for loking ahead or even peeking.
     * This ensure every token will only be lexed once.
     * Lookahead support is given by the TokenStream interface.
     */
    class Lexer {
    public:
        Lexer() = delete;
        Lexer(const llvm::MemoryBufferRef& buffer, DiagnosticReporter& reporter);
        Lexer(const SourceRange& range, DiagnosticReporter& reporter);
        Lexer(const Lexer& other) = delete;
        Lexer(Lexer&& other) = delete;
        ~Lexer() = default;

        Lexer& operator=(const Lexer& other) = delete;
        Lexer& operator=(Lexer&& other) = delete;

        /**
         * Lex the current token and advance to the next one.
         * If there is an error, it will return false.
         */
        bool Lex(Token& token) noexcept;

    private:
        bool LexIdentifier(Token& token) noexcept;
        bool LexKeyword(Token& token) noexcept;
        bool LexString(Token& token) noexcept;
        bool LexNumeric(Token& token) noexcept;
        bool LexPunctuator(Token& token) noexcept;

    private:
        SourceRange range;
        SourceLocation currentLocation;
        DiagnosticReporter& reporter;
    };

}