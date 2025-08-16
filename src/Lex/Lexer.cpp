#include <VCL/Lex/Lexer.hpp>

#include <VCL/Lex/TokenKindLookup.hpp>


VCL::Lexer::Lexer(const llvm::MemoryBufferRef& buffer) {
    range.start = (uintptr_t)buffer.getBufferStart();
    range.end = (uintptr_t)buffer.getBufferEnd();
    currentLocation = range.start;
}

VCL::Lexer::Lexer(const SourceRange& range) : range{ range } {
    currentLocation = range.start;
}

void VCL::Lexer::Seek(SourceLocation location) {
    currentLocation = location;
}

VCL::LexerError VCL::Lexer::Lex(Token& token) noexcept {
    while (currentLocation < range.end && isspace(currentLocation.GetChar()))
        ++currentLocation;
    if (currentLocation >= range.end) {
        token.kind = TokenKind::EndOfFile;
        token.range.start = currentLocation;
        token.range.end = currentLocation;
        return LexerError::None;
    }
    char currentChar = currentLocation.GetChar();
    if (isalpha(currentChar) || currentChar == '_') {
        if (LexerError err = LexIdentifier(token); err != LexerError::None) {
            currentLocation = token.range.end;
            return err;
        }
        currentLocation = token.range.end;
        return LexKeyword(token);
    } else if (currentChar == '\"') {
        LexerError err = LexString(token);
        currentLocation = token.range.end;
        return err;
    } else if (isdigit(currentChar)) {
        LexerError err = LexNumeric(token);
        currentLocation = token.range.end;
        return err;
    }

    // Must be a punctuator
    LexerError err = LexPunctuator(token);
    currentLocation = token.range.end;
    return err;
}

VCL::LexerError VCL::Lexer::LexIdentifier(Token& token) noexcept {
    token.kind = TokenKind::Identifier;
    token.range.start = currentLocation;
    token.range.end = currentLocation;
    char c = token.range.end.GetChar();
    while (token.range.end < range.end && (isalnum(c) || c == '_')) { 
        ++token.range.end; 
        c = token.range.end.GetChar();
    }
    return LexerError::None;
}

VCL::LexerError VCL::Lexer::LexKeyword(Token& token) noexcept {
    const char* startPtr = token.range.start.GetPtr();
    const char* endPtr = token.range.end.GetPtr();
    llvm::StringRef name{ startPtr, (size_t)(endPtr - startPtr) };
    if (TokenKind kind = TokenKindLookupKeyword(name); kind != TokenKind::Unknown)
        token.kind = kind;
    return LexerError::None;
}

VCL::LexerError VCL::Lexer::LexString(Token& token) noexcept {
    token.kind = TokenKind::StringLiteral;
    token.range.start = currentLocation;
    token.range.end = currentLocation;
    while (token.range.end < range.end) {
        ++token.range.end;
        char c = token.range.end.GetChar();
        if (c == '\"')
            return LexerError::None;
        if (c == '\n')
            return LexerError::UnterminatedString;
        if (c == '\\') {
            ++token.range.end;
            if (token.range.end >= range.end)
                return LexerError::UnterminatedString;
        }
    }
    return LexerError::UnterminatedString;
}

VCL::LexerError VCL::Lexer::LexNumeric(Token& token) noexcept {
    token.kind = TokenKind::NumericConstant;
    token.range.start = currentLocation;
    token.range.end = currentLocation;
    bool floating = false;
    char c = token.range.end.GetChar();
    while (token.range.end < range.end && (isdigit(c) || c == '.')) { 
        ++token.range.end; 
        c = token.range.end.GetChar();
        if (c == '.') {
            if (floating)
                return LexerError::NumericConstantTooMuchText;
            floating = true;
        }
    }
    return LexerError::None;
}

VCL::LexerError VCL::Lexer::LexPunctuator(Token& token) noexcept {
    token.kind = TokenKind::Unknown;
    token.range.start = currentLocation;
    token.range.end = currentLocation;
    uint32_t size = 0;
    while (token.range.end < range.end && size < 3) { 
        ++token.range.end;
        const char* startPtr = token.range.start.GetPtr();
        const char* endPtr = token.range.end.GetPtr();
        llvm::StringRef name{ startPtr, (size_t)(endPtr - startPtr) };
        TokenKind kind = TokenKindLookupPunctuator(name);
        if (kind != TokenKind::Unknown)
            token.kind = kind;
        ++size;
    }
    if (token.kind == TokenKind::Unknown)
        return LexerError::InvalidCharacter;
    return LexerError::None;
}