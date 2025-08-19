#include <catch2/catch_test_macros.hpp>

#include <VCL/Core/SourceManager.hpp>
#include <VCL/Core/Source.hpp>
#include <VCL/Lex/TokenStream.hpp>

#include <string>

void AssertTokenKindAndNoError(VCL::TokenStream& stream, VCL::TokenKind kind) {
    VCL::Token* token = stream.GetTok();
    REQUIRE(token != nullptr);
    REQUIRE(token->kind == kind);
}

TEST_CASE("Token Stream A Bunch Of Token", "[Core][Source]") {
    VCL::DiagnosticsEngine engine{};
    VCL::DiagnosticReporter reporter{ engine };
    VCL::SourceManager manager{ reporter };
    VCL::Source* source = manager.LoadFromMemory("int32 v = 0;");
    VCL::Lexer lexer{ source->GetBufferRef(), reporter };
    VCL::TokenStream stream{ lexer };
    
    AssertTokenKindAndNoError(stream, VCL::TokenKind::Keyword_int32);
    REQUIRE(stream.Next());
    AssertTokenKindAndNoError(stream, VCL::TokenKind::Identifier);
    REQUIRE(stream.Next());
    AssertTokenKindAndNoError(stream, VCL::TokenKind::Equal);
    REQUIRE(stream.Next());
    AssertTokenKindAndNoError(stream, VCL::TokenKind::NumericConstant);
    REQUIRE(stream.Next());
    AssertTokenKindAndNoError(stream, VCL::TokenKind::Semicolon);
    REQUIRE(stream.Next());
    AssertTokenKindAndNoError(stream, VCL::TokenKind::EndOfFile);
    //It should still send EndOfFile even with the next token
    REQUIRE(stream.Next());
    AssertTokenKindAndNoError(stream, VCL::TokenKind::EndOfFile);
}

TEST_CASE("Token Stream Save & Restore", "[Core][Source]") {
    VCL::DiagnosticsEngine engine{};
    VCL::DiagnosticReporter reporter{ engine };
    VCL::SourceManager manager{ reporter };
    VCL::Source* source = manager.LoadFromMemory("int32 v = 0;");
    VCL::Lexer lexer{ source->GetBufferRef(), reporter };
    VCL::TokenStream stream{ lexer };
    
    AssertTokenKindAndNoError(stream, VCL::TokenKind::Keyword_int32);
    REQUIRE(stream.Next());
    AssertTokenKindAndNoError(stream, VCL::TokenKind::Identifier);
    REQUIRE(stream.Next());

    stream.Save();

    AssertTokenKindAndNoError(stream, VCL::TokenKind::Equal);
    REQUIRE(stream.Next());
    AssertTokenKindAndNoError(stream, VCL::TokenKind::NumericConstant);
    REQUIRE(stream.Next());
    AssertTokenKindAndNoError(stream, VCL::TokenKind::Semicolon);
    REQUIRE(stream.Next());
    AssertTokenKindAndNoError(stream, VCL::TokenKind::EndOfFile);

    stream.Restore();

    AssertTokenKindAndNoError(stream, VCL::TokenKind::Equal);
    REQUIRE(stream.Next());
    AssertTokenKindAndNoError(stream, VCL::TokenKind::NumericConstant);
    REQUIRE(stream.Next());
    AssertTokenKindAndNoError(stream, VCL::TokenKind::Semicolon);
    REQUIRE(stream.Next());
    AssertTokenKindAndNoError(stream, VCL::TokenKind::EndOfFile);
}

TEST_CASE("Token Stream Save & Commit", "[Core][Source]") {
    VCL::DiagnosticsEngine engine{};
    VCL::DiagnosticReporter reporter{ engine };
    VCL::SourceManager manager{ reporter };
    VCL::Source* source = manager.LoadFromMemory("int32 v = 0;");
    VCL::Lexer lexer{ source->GetBufferRef(), reporter };
    VCL::TokenStream stream{ lexer };
    
    AssertTokenKindAndNoError(stream, VCL::TokenKind::Keyword_int32);
    REQUIRE(stream.Next());
    AssertTokenKindAndNoError(stream, VCL::TokenKind::Identifier);
    REQUIRE(stream.Next());

    stream.Save();

    AssertTokenKindAndNoError(stream, VCL::TokenKind::Equal);
    REQUIRE(stream.Next());
    AssertTokenKindAndNoError(stream, VCL::TokenKind::NumericConstant);
    REQUIRE(stream.Next());
    AssertTokenKindAndNoError(stream, VCL::TokenKind::Semicolon);
    REQUIRE(stream.Next());
    AssertTokenKindAndNoError(stream, VCL::TokenKind::EndOfFile);

    stream.Commit();

    AssertTokenKindAndNoError(stream, VCL::TokenKind::EndOfFile);
    REQUIRE(stream.Next());
    AssertTokenKindAndNoError(stream, VCL::TokenKind::EndOfFile);
}