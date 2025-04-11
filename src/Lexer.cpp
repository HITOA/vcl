#include <VCL/Lexer.hpp>

#include <cstring>
#include <algorithm>


bool VCL::Lexer::Tokenize(std::shared_ptr<Source> source) {
    uint32_t offset = 0;
    uint32_t position = 0;
    uint32_t line = 0;

    std::string_view source_view = source->source;

    while (offset < source_view.length()) {
        while (SkipWhitespace(source_view, offset, position, line) || SkipComment(source_view, offset, position, line)) {}

        std::string_view currentSource{ source_view.data() + offset, source_view.length() - offset };
        TokenType currentTokenType;
        uint32_t currentTokenSize;

        if (!TokenizeLiteral(currentSource, currentTokenType, currentTokenSize))
            if (!TokenizeIdentifier(currentSource, currentTokenType, currentTokenSize))
                TokenizePunctuator(currentSource, currentTokenType, currentTokenSize);

        std::string_view currentTokenName{ currentSource.data(), currentTokenSize };

        if (currentTokenType == TokenType::IDENTIFIER)
            TokenizeKeyword(currentTokenName, currentTokenType);

        if (currentTokenType != TokenType::UNDEFINED) {
            tokens.emplace_back(Token{
                currentTokenType,
                currentTokenName,
                SourceLocation{
                    source,
                    position,
                    line,
                    currentTokenSize
                }
            });
        } else {
            return false;
        }

        offset += currentTokenSize;
        position += currentTokenSize;
    }

    tokens.emplace_back(Token{
        TokenType::ENDOFFILE,
        source_view,
        SourceLocation{
            source,
            position,
            line,
            1
        }
    });

    return true;
}

VCL::Token VCL::Lexer::Consume() {
    Token token = tokens[cursor++];
    return token;
}

VCL::Token VCL::Lexer::Peek(int32_t offset) {
    return tokens[std::clamp(cursor + offset, 0u, (uint32_t)tokens.size())];
}

bool VCL::Lexer::ConsumeIf(TokenType type) {
    Token token = tokens[cursor];
    if (token.type == type) {
        ++cursor;
        return true;
    }
    return false;
}

bool VCL::Lexer::TokenizePunctuator(std::string_view source, TokenType& type, uint32_t& size) {
#undef DEF
#define DEF(name, symbol, ...) { symbol, TokenType::name },
    static struct PunctuatorTokenType {
        const char* punctuator;
        TokenType type;
    } punctuatorTokenType[] = {
        UNARY_OPERATOR_DEF
        BINARY_OPERATOR_DEF
        PUNCTUATOR_DEF
        { "", TokenType::UNDEFINED }
    };

    uint32_t currentTokenTypeLength = 0;

    for (uint32_t i = 0; i < sizeof(punctuatorTokenType) / sizeof(PunctuatorTokenType); ++i) {
        uint32_t punctuatorLength = strlen(punctuatorTokenType[i].punctuator);
        if (source.length() < punctuatorLength || punctuatorLength <= currentTokenTypeLength)
            continue;
        if (memcmp(source.data(), punctuatorTokenType[i].punctuator, punctuatorLength) == 0) {
            type = punctuatorTokenType[i].type;
            currentTokenTypeLength = punctuatorLength;
        }
    }

    size = currentTokenTypeLength;
    return currentTokenTypeLength != 0;
}

bool VCL::Lexer::TokenizeIdentifier(std::string_view source, TokenType& type, uint32_t& size) {
    if (!isalpha(source[0]) && source[0] != '_') {
        type = TokenType::UNDEFINED;
        size = 0;
        return false;
    }

    for (size_t i = 0; i < source.length(); ++i) {
        if (!isalnum(source[i]) && source[i] != '_') {
            type = TokenType::IDENTIFIER;
            size = i;
            return true;
        }
    }

    type = TokenType::IDENTIFIER;
    size = source.length();
    return true;
}

bool VCL::Lexer::TokenizeLiteral(std::string_view source, TokenType& type, uint32_t& size) {
    if (!isdigit(source[0])) {
        type = TokenType::UNDEFINED;
        size = 0;
        return false;
    }

    bool isFloating = false;

    for (size_t i = 0; i < source.length(); ++i) {
        if (!isdigit(source[i]) && source[i] != '.') {
            type = isFloating ? TokenType::LITERALFLOAT : TokenType::LITERALINT;
            size = i;
            return true;
        }
        if (source[i] == '.')
            isFloating = true;
    }

    type = isFloating ? TokenType::LITERALFLOAT : TokenType::LITERALINT;
    size = source.length();
    return true;
}

void VCL::Lexer::TokenizeKeyword(std::string_view identifier, TokenType& type) {
#undef DEF
#define DEF(name, symbol, ...) { symbol, TokenType::name },
    static struct KeywordTokenType {
        const char* keyword;
        TokenType type;
    } keywordTokenType[] = {
        TYPE_DEF
        TYPE_QUALIFIER_DEF
        KEYWORD_DEF
        { "", TokenType::UNDEFINED }
    };

    for (uint32_t i = 0; i < sizeof(keywordTokenType) / sizeof(KeywordTokenType); ++i) {
        uint32_t keywordLength = strlen(keywordTokenType[i].keyword);
        if (identifier.length() != keywordLength)
            continue;
        if (memcmp(identifier.data(), keywordTokenType[i].keyword, keywordLength) == 0) {
            type = keywordTokenType[i].type;
            return;
        }
    }
}

bool VCL::Lexer::SkipWhitespace(std::string_view source, uint32_t& offset, uint32_t& position, uint32_t& line) {
    if (!isspace(source[offset]))
        return false;
    while (offset < source.length() && isspace(source[offset])) {
        if (source[offset] == '\n') {
            ++line;
            position = 0;
        } else {
            ++position;
        }
        ++offset;
    }
    return true;
}

bool VCL::Lexer::SkipComment(std::string_view source, uint32_t& offset, uint32_t& position, uint32_t& line) {
    if (source.length() <= offset + 1)
        return false;

    if (source[offset] != '/')
        return false;
    
    std::string ends;

    if (source[offset + 1] == '*') {
        ends = "*/";
    } else if (source[offset + 1] == '/') {
        ends = "\n";
    } else {
        return false;
    }
    
    offset += 2;

    while ((offset + ends.size()) < source.length() && memcmp(&source[offset], ends.data(), ends.size()) != 0) {
        if (source[offset] == '\n') {
            ++line;
            position = 0;
        } else {
            ++position;
        }
        ++offset;
    }
    
    offset += ends.size();
    if (ends == "\n") {
        ++line;
        position = 0;
    }

    return true;
}