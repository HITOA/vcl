#include <catch2/catch_test_macros.hpp>

#include <VCL/Core/SourceManager.hpp>
#include <VCL/Core/Source.hpp>
#include <VCL/Lex/TokenStream.hpp>
#include <VCL/Frontend/CompilerContext.hpp>

#include "../Common/ExpectedDiagnostic.hpp"

#include <string>

void AssertTokenKindAndNoError(VCL::TokenStream& stream, VCL::TokenKind kind) {
    VCL::Token* token = stream.GetTok();
    REQUIRE(token != nullptr);
    VCL::TokenKind ckind = token->kind;
    if (ckind == VCL::TokenKind::Identifier)
        ckind = token->identifier->GetTokenKind();
    REQUIRE(ckind == kind);
}

TEST_CASE("Token Stream A Bunch Of Token", "[Lex][TokenStream]") {
    ExpectedNoDiagnostic consumer{};
    VCL::CompilerContext cc{};
    cc.GetInvocation()->GetDiagnosticOptions().SetDiagnosticConsumer(&consumer);
    cc.CreateDiagnosticEngine();
    cc.CreateIdentifierTable();
    cc.CreateSourceManager();
    VCL::Source* source = cc.GetSourceManager().LoadFromMemory("int32 v = 0;");
    REQUIRE(source != nullptr);
    VCL::Lexer lexer{ source->GetBufferRef(), cc.GetDiagnosticReporter(), cc.GetIdentifierTable() };
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

TEST_CASE("Token Stream Save & Restore", "[Lex][TokenStream]") {
    ExpectedNoDiagnostic consumer{};
    VCL::CompilerContext cc{};
    cc.GetInvocation()->GetDiagnosticOptions().SetDiagnosticConsumer(&consumer);
    cc.CreateDiagnosticEngine();
    cc.CreateIdentifierTable();
    cc.CreateSourceManager();
    VCL::Source* source = cc.GetSourceManager().LoadFromMemory("int32 v = 0;");
    REQUIRE(source != nullptr);
    VCL::Lexer lexer{ source->GetBufferRef(), cc.GetDiagnosticReporter(), cc.GetIdentifierTable() };
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

TEST_CASE("Token Stream Save & Commit", "[Lex][TokenStream]") {
    ExpectedNoDiagnostic consumer{};
    VCL::CompilerContext cc{};
    cc.GetInvocation()->GetDiagnosticOptions().SetDiagnosticConsumer(&consumer);
    cc.CreateDiagnosticEngine();
    cc.CreateIdentifierTable();
    cc.CreateSourceManager();
    VCL::Source* source = cc.GetSourceManager().LoadFromMemory("int32 v = 0;");
    REQUIRE(source != nullptr);
    VCL::Lexer lexer{ source->GetBufferRef(), cc.GetDiagnosticReporter(), cc.GetIdentifierTable() };
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