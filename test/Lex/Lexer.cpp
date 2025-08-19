#include <catch2/catch_test_macros.hpp>

#include <VCL/Core/SourceManager.hpp>
#include <VCL/Core/Source.hpp>
#include <VCL/Lex/Lexer.hpp>


void LexerTokenKindTest(VCL::TokenKind kind, llvm::StringRef value) {
    VCL::DiagnosticsEngine engine{};
    VCL::DiagnosticReporter reporter{ engine };
    VCL::SourceManager manager{ reporter };
    VCL::Source* source = manager.LoadFromMemory(value);
    REQUIRE(source != nullptr);
    VCL::Lexer lexer{ source->GetBufferRef(), reporter };
    VCL::Token token{};
    bool r = lexer.Lex(token);
    REQUIRE(r == true);
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
