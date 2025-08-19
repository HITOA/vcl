#pragma once

#include <VCL/Lex/Lexer.hpp>

#include <vector>
#include <stack>
#include <expected>


namespace VCL {

    /**
     * Provide an interface with lookahead, save, restore, and commit functionality.
     */
    class TokenStream {
    public:
        TokenStream() = delete;
        TokenStream(Lexer& lexer);
        TokenStream(const TokenStream& other) = delete;
        TokenStream(TokenStream&& other) = default;
        ~TokenStream() = default;

        TokenStream& operator=(const TokenStream& other) = delete;
        TokenStream& operator=(TokenStream&& other) = default;

        /** 
         * Return the nth lexed token from the current position or a LexerError. 
         * n = 0 will return the current lexed token.
         * n = 1 will return the next token after the current one.
         * There is no limit to n and it is expected to return the last token even if it goes past it.
         * This function will return nullptr if it failes.
         */
        Token* GetTok(uint32_t n = 0);

        /** Lex the next nth token(s) and return itself for chaining. */
        bool Next(uint32_t n = 1);

        /** Push the current position to the stack. */
        void Save();

        /** Restore the last position pushed on the stack. */
        void Restore();

        /** Commit and pop the last position. */
        void Commit();

    private:
        /** Grow the internal buffer to hold the next nth tokens and lex them. */
        bool GrowBufferAndLexTokens(uint32_t n);
        /** Shrink the back of the buffer for nth elements */
        void ShrinkBackBy(uint32_t n);

    private:
        uint32_t currentIndex = 0;
        Lexer& lexer;
        std::vector<Token> buffer{};
        std::stack<uint32_t> savePoints{};
    };

}