#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <VCL/Definition.hpp>

#undef DEF
#define DEF(name, symbol, ...) name,


namespace VCL {

    /**
     * @brief This is all the token type the lexer and other class are using. Most of them come from Definition.hpp
     */
    enum class TokenType {
        UNDEFINED,
        ENDOFFILE,
        IDENTIFIER,
        LITERALINT,
        LITERALFLOAT,

        TYPE_DEF
        TYPE_QUALIFIER_DEF
        KEYWORD_DEF
        UNARY_OPERATOR_DEF
        BINARY_OPERATOR_DEF
        PUNCTUATOR_DEF
        MAX
    };

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
         * @brief provide a view on the source where the token belong.
         */
        std::string_view source;
        /**
         * @brief specify the horizontal position of the token in the source.
         */
        uint32_t position;
        /**
         * @brief specify the vertical position (line) of the token in the source. 
         */
        uint32_t line;
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

        bool Tokenize(std::string_view source);
        
        Token Consume();
        Token Peek(int32_t offset = 0);

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