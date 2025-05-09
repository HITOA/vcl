#pragma once

#include <VCL/Definition.hpp>
#include <VCL/Source.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#undef DEF
#define DEF(name, symbol, ...) name,


namespace VCL {
    /**
     * @brief This class represents a token for the lexer.
     */
    struct Token {
        /**
         * @brief specify the type of this Token.
         */
        TokenType type;
        /**
         * @brief provide a view on the source of the token's name.
         */
        std::string_view name;
        /**
         * @brief provide information on the token's location in the source file.
         */
        SourceLocation location;
    };

    /**
     * @brief Tokenize a while source file into a list of token.
     * 
     * This class is mainly used by the Parser class and isn't really made to be used as is.
     * Does nothing regarding preprocessing. Also skip all whitespaces and comments.
     */
    class Lexer {
    public:
        Lexer() = default;
        ~Lexer() = default;

        bool Tokenize(std::shared_ptr<Source> source);
        
        Token Consume();
        Token Peek(int32_t offset = 0);
        
        bool ConsumeIf(TokenType type);

        uint32_t GetCursor() const;
        void SetCursor(uint32_t cursor);

    private:
        bool TokenizePunctuator(std::string_view source, TokenType& type, uint32_t& size);
        bool TokenizeIdentifier(std::string_view source, TokenType& type, uint32_t& size);
        bool TokenizeLiteral(std::string_view source, TokenType& type, uint32_t& size);
        void TokenizeKeyword(std::string_view identifier, TokenType& type);

        bool SkipWhitespace(std::string_view source, uint32_t& offset, uint32_t& position, uint32_t& line);
        bool SkipComment(std::string_view source, uint32_t& offset, uint32_t& position, uint32_t& line);

    private:
        std::vector<Token> tokens = {};
        uint32_t cursor = 0;
    };

}