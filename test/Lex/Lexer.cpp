#include <catch2/catch_test_macros.hpp>

#include <VCL/Core/Source.hpp>
#include <VCL/Lex/Lexer.hpp>


void LexerTokenKindTest(VCL::TokenKind kind, llvm::StringRef value) {
    VCL::Handle<VCL::Source> source = *VCL::Source::LoadFromMemory(value);
    VCL::Lexer lexer{ source->GetBufferRef() };
    VCL::Token token{};
    VCL::LexerError err = lexer.Lex(token);
    INFO(VCL::ToString(err));
    REQUIRE(token.kind == kind);
}

void LexerTokenErrorTest(VCL::LexerError error, llvm::StringRef value) {
    VCL::Handle<VCL::Source> source = *VCL::Source::LoadFromMemory(value);
    VCL::Lexer lexer{ source->GetBufferRef() };
    VCL::Token token{};
    VCL::LexerError err = lexer.Lex(token);
    REQUIRE(err == error);
}


TEST_CASE("Lexer Token Check EndOfFile", "[Lexer][Token]") {
    LexerTokenKindTest(VCL::TokenKind::EndOfFile, "");
}

TEST_CASE("Lexer Token Check Identifier", "[Lexer][Token]") {
    LexerTokenKindTest(VCL::TokenKind::Identifier, "_CustomIdentifier1234");
    LexerTokenKindTest(VCL::TokenKind::Identifier, "custom1234_Identifier");
}

TEST_CASE("Lexer Token Check String Literal", "[Lexer][Token]") {
    LexerTokenKindTest(VCL::TokenKind::StringLiteral, "\"Some String \\n Literal\"");
}

TEST_CASE("Lexer Token Check Numeric Constant", "[Lexer][Token]") {
    LexerTokenKindTest(VCL::TokenKind::NumericConstant, "3409827983");
    LexerTokenKindTest(VCL::TokenKind::NumericConstant, "3498.3583");
}

#define KEYWORD(x) TEST_CASE("Lexer Token Check " #x, "[Lexer][Token]") { \
    LexerTokenKindTest(VCL::TokenKind::Keyword_##x, #x);\
}
#include <VCL/Core/TokenKind.def>

#define PUNCTUATOR(x, y) TEST_CASE("Lexer Token Check " #x, "[Lexer][Token]") { \
    LexerTokenKindTest(VCL::TokenKind::x, y);\
}
#include <VCL/Core/TokenKind.def>

TEST_CASE("Lexer Token Check Unterminated String", "[Lexer][Token]") {
    LexerTokenErrorTest(VCL::LexerError::UnterminatedString, "\"Unterminated String");
    LexerTokenErrorTest(VCL::LexerError::UnterminatedString, "\"Unterminated String\n\"");
}

TEST_CASE("Lexer Token Check Invalid Character", "[Lexer][Token]") {
    LexerTokenErrorTest(VCL::LexerError::InvalidCharacter, "§");
}

TEST_CASE("Lexer Token Check Numeric Constant Too Much Text", "[Lexer][Token]") {
    LexerTokenErrorTest(VCL::LexerError::NumericConstantTooMuchText, "234.2302.423");
}