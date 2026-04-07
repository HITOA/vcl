#include <catch2/catch_test_macros.hpp>

#include <VCL/Core/SourceManager.hpp>
#include <VCL/Core/Source.hpp>
#include <VCL/Lex/Lexer.hpp>
#include <VCL/Frontend/CompilerContext.hpp>

#include "../Common/ExpectedDiagnostic.hpp"


void LexerTokenKindTest(VCL::TokenKind kind, llvm::StringRef value) {
    ExpectedNoDiagnostic consumer{};
    VCL::CompilerContext cc{};
    cc.GetInvocation()->GetDiagnosticOptions().SetDiagnosticConsumer(&consumer);
    cc.CreateDiagnosticEngine();
    cc.CreateIdentifierTable();
    cc.CreateSourceManager();
    VCL::Source* source = cc.GetSourceManager().LoadFromMemory(value);
    REQUIRE(source != nullptr);
    VCL::Lexer lexer{ source->GetBufferRef(), cc.GetDiagnosticReporter(), cc.GetIdentifierTable() };
    VCL::Token token{};
    bool r = lexer.Lex(token);
    REQUIRE(r == true);
    VCL::TokenKind ckind = token.kind;
    if (ckind == VCL::TokenKind::Identifier)
        ckind = token.identifier->GetTokenKind();
    REQUIRE(ckind == kind);
}


TEST_CASE("Lexer Token Check EndOfFile", "[Lex][Lexer]") {
    LexerTokenKindTest(VCL::TokenKind::EndOfFile, "");
}

TEST_CASE("Lexer Token Check Identifier", "[Lex][Lexer]") {
    LexerTokenKindTest(VCL::TokenKind::Identifier, "_CustomIdentifier1234");
    LexerTokenKindTest(VCL::TokenKind::Identifier, "custom1234_Identifier");
}

TEST_CASE("Lexer Token Check String Literal", "[Lex][Lexer]") {
    LexerTokenKindTest(VCL::TokenKind::StringLiteral, "\"Some String \\n Literal\"");
}

TEST_CASE("Lexer Token Check Numeric Constant", "[Lex][Lexer]") {
    LexerTokenKindTest(VCL::TokenKind::NumericConstant, "3409827983");
    LexerTokenKindTest(VCL::TokenKind::NumericConstant, "3498.3583");
}

#define KEYWORD(x) TEST_CASE("Lexer Token Check " #x, "[Lex][Lexer]") { \
    LexerTokenKindTest(VCL::TokenKind::Keyword_##x, #x);\
}
#include <VCL/Core/TokenKind.def>

#define PUNCTUATOR(x, y) TEST_CASE("Lexer Token Check " #x, "[Lex][Lexer]") { \
    LexerTokenKindTest(VCL::TokenKind::x, y);\
}
#include <VCL/Core/TokenKind.def>
