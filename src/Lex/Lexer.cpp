#include <VCL/Lex/Lexer.hpp>

#include <VCL/Lex/TokenKindLookup.hpp>


VCL::Lexer::Lexer(const llvm::MemoryBufferRef& buffer, CompilerContext& cc) : cc{ cc } {
    range.start = (uintptr_t)buffer.getBufferStart();
    range.end = (uintptr_t)buffer.getBufferEnd();
    currentLocation = range.start;
}

VCL::Lexer::Lexer(const SourceRange& range, CompilerContext& cc) : range{ range }, cc{ cc } {
    currentLocation = range.start;
}

bool VCL::Lexer::Lex(Token& token) noexcept {
    token.identifier = nullptr;
    token.isFloatingPoint = false;
    while (SkipComment() || SkipWhitespace()) {}
    if (currentLocation >= range.end) {
        token.kind = TokenKind::EndOfFile;
        token.range.start = currentLocation;
        token.range.end = currentLocation;
        return true;
    }
    char currentChar = currentLocation.GetChar();
    if (isalpha(currentChar) || currentChar == '_') {
        bool r = LexIdentifier(token);
        currentLocation = token.range.end;
        return r;
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

    const char* startPtr = token.range.start.GetPtr();
    const char* endPtr = token.range.end.GetPtr();
    llvm::StringRef name{ startPtr, (size_t)(endPtr - startPtr) };
    
    token.identifier = cc.GetIdentifierTable().Get(name);

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
            return cc.GetDiagnosticReporter().Error(Diagnostic::UnterminatedString, 
                std::string{ token.range.start.GetPtr(), token.range.end.GetPtr() })
                .AddHint(DiagnosticHint{ token.range })
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
        if (c == '\\') {
            ++token.range.end;
            if (token.range.end >= range.end)
                return cc.GetDiagnosticReporter().Error(Diagnostic::UnterminatedString, 
                    std::string{ token.range.start.GetPtr(), token.range.end.GetPtr() })
                    .AddHint(DiagnosticHint{ token.range })
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .Report();
        }
    }
    return cc.GetDiagnosticReporter().Error(Diagnostic::UnterminatedString, 
                std::string{ token.range.start.GetPtr(), token.range.end.GetPtr() })
                .AddHint(DiagnosticHint{ token.range })
                .SetCompilerInfo(__FILE__, __func__, __LINE__)
                .Report();
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
                return cc.GetDiagnosticReporter().Error(Diagnostic::NumericConstantTooMuchText, 
                    std::string{ token.range.start.GetPtr(), token.range.end.GetPtr() })
                    .AddHint(DiagnosticHint{ token.range })
                    .SetCompilerInfo(__FILE__, __func__, __LINE__)
                    .Report();
            floating = true;
        }
    }
    token.isFloatingPoint = floating;
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
        return cc.GetDiagnosticReporter().Error(Diagnostic::InvalidCharacter, std::string{ token.range.start.GetPtr(), token.range.end.GetPtr() })
            .AddHint(DiagnosticHint{ token.range })
            .SetCompilerInfo(__FILE__, __func__, __LINE__)
            .Report();
    token.range.end = okEndRange;
    return true;
}

bool VCL::Lexer::SkipWhitespace() noexcept {
    bool skipped = false;
    while (currentLocation < range.end && isspace(currentLocation.GetChar())) {
        ++currentLocation;
        skipped = true;
    }
    return skipped;
}

bool VCL::Lexer::SkipComment() noexcept {
    if (currentLocation + 2 >= range.end || currentLocation.GetChar() != '/')
        return false;
    std::string end{};
    if ((currentLocation + 1).GetChar() == '/')
        end = "\n";
    else if ((currentLocation + 1).GetChar() == '*')
        end = "*/";
    else
        return false;
    currentLocation += 2;
    while ((currentLocation + end.size()) < range.end && memcmp(currentLocation.GetPtr(), end.data(), end.size()))
        ++currentLocation;
    currentLocation += end.size();
    return true;
}