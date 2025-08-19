#include <VCL/Lex/Lexer.hpp>

#include <VCL/Lex/TokenKindLookup.hpp>


VCL::Lexer::Lexer(const llvm::MemoryBufferRef& buffer, DiagnosticReporter& reporter) : reporter{ reporter } {
    range.start = (uintptr_t)buffer.getBufferStart();
    range.end = (uintptr_t)buffer.getBufferEnd();
    currentLocation = range.start;
}

VCL::Lexer::Lexer(const SourceRange& range, DiagnosticReporter& reporter) : range{ range }, reporter{ reporter } {
    currentLocation = range.start;
}

bool VCL::Lexer::Lex(Token& token) noexcept {
    while (currentLocation < range.end && isspace(currentLocation.GetChar()))
        ++currentLocation;
    if (currentLocation >= range.end) {
        token.kind = TokenKind::EndOfFile;
        token.range.start = currentLocation;
        token.range.end = currentLocation;
        return true;
    }
    char currentChar = currentLocation.GetChar();
    if (isalpha(currentChar) || currentChar == '_') {
        if (!LexIdentifier(token)) {
            currentLocation = token.range.end;
            return false;
        }
        currentLocation = token.range.end;
        return LexKeyword(token);
    } else if (currentChar == '\"') {
        bool r = LexString(token);
        currentLocation = token.range.end;
        return r;
    } else if (isdigit(currentChar)) {
        bool r = LexNumeric(token);
        currentLocation = token.range.end;
        return r;
    }

    // Must be a punctuator
    bool r = LexPunctuator(token);
    currentLocation = token.range.end;
    return r;
}

bool VCL::Lexer::LexIdentifier(Token& token) noexcept {
    token.kind = TokenKind::Identifier;
    token.range.start = currentLocation;
    token.range.end = currentLocation;
    char c = token.range.end.GetChar();
    while (token.range.end < range.end && (isalnum(c) || c == '_')) { 
        ++token.range.end; 
        c = token.range.end.GetChar();
    }
    return true;
}

bool VCL::Lexer::LexKeyword(Token& token) noexcept {
    const char* startPtr = token.range.start.GetPtr();
    const char* endPtr = token.range.end.GetPtr();
    llvm::StringRef name{ startPtr, (size_t)(endPtr - startPtr) };
    if (TokenKind kind = TokenKindLookupKeyword(name); kind != TokenKind::Unknown)
        token.kind = kind;
    return true;
}

bool VCL::Lexer::LexString(Token& token) noexcept {
    token.kind = TokenKind::StringLiteral;
    token.range.start = currentLocation;
    token.range.end = currentLocation;
    while (token.range.end < range.end) {
        ++token.range.end;
        char c = token.range.end.GetChar();
        if (c == '\"')
            return true;
        if (c == '\n')
            return reporter.Error(Diagnostic::UnterminatedString, 
                std::string{ token.range.start.GetPtr(), token.range.end.GetPtr() });
        if (c == '\\') {
            ++token.range.end;
            if (token.range.end >= range.end)
                return reporter.Error(Diagnostic::UnterminatedString, 
                    std::string{ token.range.start.GetPtr(), token.range.end.GetPtr() });
        }
    }
    return reporter.Error(Diagnostic::UnterminatedString, 
                std::string{ token.range.start.GetPtr(), token.range.end.GetPtr() });
}

bool VCL::Lexer::LexNumeric(Token& token) noexcept {
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
                return reporter.Error(Diagnostic::NumericConstantTooMuchText, 
                    std::string{ token.range.start.GetPtr(), token.range.end.GetPtr() });;
            floating = true;
        }
    }
    return true;
}

bool VCL::Lexer::LexPunctuator(Token& token) noexcept {
    token.kind = TokenKind::Unknown;
    token.range.start = currentLocation;
    token.range.end = currentLocation;
    SourceLocation okEndRange = currentLocation;
    uint32_t size = 0;
    while (token.range.end < range.end && size < 3) { 
        ++token.range.end;
        const char* startPtr = token.range.start.GetPtr();
        const char* endPtr = token.range.end.GetPtr();
        llvm::StringRef name{ startPtr, (size_t)(endPtr - startPtr) };
        TokenKind kind = TokenKindLookupPunctuator(name);
        if (kind != TokenKind::Unknown) {
            token.kind = kind;
            okEndRange = token.range.end;
        }
        ++size;
    }
    if (token.kind == TokenKind::Unknown)
        return reporter.Error(Diagnostic::InvalidCharacter, 
            std::string{ token.range.start.GetPtr(), token.range.end.GetPtr() });;
    token.range.end = okEndRange;
    return true;
}