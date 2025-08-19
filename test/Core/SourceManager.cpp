#include <catch2/catch_test_macros.hpp>

#include <VCL/Core/SourceManager.hpp>
#include <VCL/Core/Source.hpp>


TEST_CASE("Source File Found", "[Core][Source]") {
    VCL::DiagnosticsEngine engine{};
    VCL::DiagnosticReporter reporter{ engine };
    VCL::SourceManager manager{ reporter };
    VCL::Source* source = manager.LoadFromDisk("VCL/empty.vcl");
    REQUIRE(source != nullptr);
}

TEST_CASE("Source File Not Found", "[Core][Source]") {
    VCL::DiagnosticsEngine engine{};
    VCL::DiagnosticReporter reporter{ engine };
    VCL::SourceManager manager{ reporter };
    VCL::Source* source = manager.LoadFromDisk("filethatdoesnotexists.vcl");
    REQUIRE(source == nullptr);
}