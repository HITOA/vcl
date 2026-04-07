#include <catch2/catch_test_macros.hpp>

#include <VCL/Core/SourceManager.hpp>
#include <VCL/Core/Source.hpp>
#include <VCL/Frontend/CompilerContext.hpp>

#include "../Common/ExpectedDiagnostic.hpp"


TEST_CASE("Source File Found", "[Core][Source]") {
    ExpectedNoDiagnostic consumer{};
    VCL::CompilerContext cc{};
    cc.GetInvocation()->GetDiagnosticOptions().SetDiagnosticConsumer(&consumer);
    cc.CreateDiagnosticEngine();
    cc.CreateSourceManager();
    VCL::Source* source = cc.GetSourceManager().LoadFromDisk("VCL/empty.vcl");
    REQUIRE(source != nullptr);
}

TEST_CASE("Source File Not Found", "[Core][Source]") {
    ExpectedDiagnostic<VCL::Diagnostic::FileNotFound> consumer{};
    VCL::CompilerContext cc{};
    cc.GetInvocation()->GetDiagnosticOptions().SetDiagnosticConsumer(&consumer);
    cc.CreateDiagnosticEngine();
    cc.CreateSourceManager();
    VCL::Source* source = cc.GetSourceManager().LoadFromDisk("doesnotexist.vcl");
    REQUIRE(source == nullptr);
    consumer.Require();
}