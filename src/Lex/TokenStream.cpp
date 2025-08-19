#include <VCL/Lex/TokenStream.hpp>


VCL::TokenStream::TokenStream(Lexer& lexer) : lexer{ lexer } {
    GrowBufferAndLexTokens(1);
}

VCL::Token* VCL::TokenStream::GetTok(uint32_t n) {
    if (currentIndex + n >= buffer.size())
        if (!GrowBufferAndLexTokens(n))
            return nullptr;
    return &buffer[currentIndex + n];
}

bool VCL::TokenStream::Next(uint32_t n) {
    if (savePoints.empty()) {
        if (n == 1 && buffer.size() == 1) {
            return lexer.Lex(buffer[0]);
        } else if (currentIndex + n < buffer.size()) {
            currentIndex += n;
            ShrinkBackBy(currentIndex);
            return true;
        } else {
            if (!GrowBufferAndLexTokens(n))
                return false;
            currentIndex += n;
            ShrinkBackBy(currentIndex);
            return true;
        }
    } else {
        if (currentIndex + n >= buffer.size())
            if (!GrowBufferAndLexTokens(n))
                return false;
        currentIndex += n;
        return true;
    }
}

void VCL::TokenStream::Save() {
    savePoints.push(currentIndex);
}

void VCL::TokenStream::Restore() {
    currentIndex = savePoints.top();
    savePoints.pop();
}

void VCL::TokenStream::Commit() {
    savePoints.pop();
    if (savePoints.empty())
        ShrinkBackBy(currentIndex);
}

bool VCL::TokenStream::GrowBufferAndLexTokens(uint32_t n) {
    size_t c = buffer.size();
    buffer.resize(c + n);
    for (size_t i = c; i < buffer.size(); ++i)
        if (!lexer.Lex(buffer[i]))
            return false;
    return true;
}

void VCL::TokenStream::ShrinkBackBy(uint32_t n) {
    buffer.erase(buffer.begin(), buffer.begin() + n);
    currentIndex -= n;
}