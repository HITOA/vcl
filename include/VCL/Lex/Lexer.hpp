#pragma once

#include <VCL/Core/LexerError.hpp>
#include <VCL/Core/SourceLocation.hpp>
#include <VCL/Lex/Token.hpp>

#include <llvm/Support/MemoryBufferRef.h>
#include <llvm/ADT/StringMap.h>


namespace VCL {

    /**
     * Provide a simple interface to tokenize a given Source.
     */
    class Lexer {
    public:
        Lexer() = delete;
        Lexer(const llvm::MemoryBufferRef& buffer);
        Lexer(const SourceRange& range);
        Lexer(const Lexer& lexer) = delete;
        Lexer(Lexer&& lexer) = delete;
        ~Lexer() = default;

        Lexer& operator=(const Lexer& lexer) = delete;
        Lexer& operator=(Lexer&& lexer) = delete;

        void Seek(SourceLocation location);

        /**
         * Lex the current token and advance to the next one.
         * If there is an error, it will return it in the form of a LexerError.
         * LexerError evaluate to true of there is an error.
         */
        LexerError Lex(Token& token) noexcept;

    private:
        LexerError LexIdentifier(Token& token) noexcept;
        LexerError LexKeyword(Token& token) noexcept;
        LexerError LexString(Token& token) noexcept;
        LexerError LexNumeric(Token& token) noexcept;
        LexerError LexPunctuator(Token& token) noexcept;

    private:
        SourceRange range{};
        SourceLocation currentLocation{};
    };

}