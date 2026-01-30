#include <VCL/Lex/TokenKindLookup.hpp>

#include <llvm/ADT/StringMap.h>


VCL::TokenKind VCL::TokenKindLookupKeyword(llvm::StringRef value) {
    static llvm::StringMap<TokenKind> keywordMap{
        #define KEYWORD(x) { #x, TokenKind::Keyword_##x },
        #include <VCL/Core/TokenKind.def>
    };

    if (keywordMap.count(value))
        return keywordMap.lookup(value);
    return TokenKind::Unknown;
}

VCL::TokenKind VCL::TokenKindLookupPunctuator(llvm::StringRef value) {
    static llvm::StringMap<TokenKind> punctuatorMap{
        #define PUNCTUATOR(x, y) { y, TokenKind::x },
        #include <VCL/Core/TokenKind.def>
    };

    if (punctuatorMap.count(value))
        return punctuatorMap.lookup(value);
    return TokenKind::Unknown;
}