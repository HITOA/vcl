#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <vcl/definition.hpp>

#undef DEF
#define DEF(name, symbol, ...) name,


namespace VCL {

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

    struct Token {
        TokenType type;
        std::string_view name;
        uint32_t position;
        uint32_t line;
    };

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