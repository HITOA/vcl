#include <catch2/catch_test_macros.hpp>

#include <VCL/Core/Source.hpp>


TEST_CASE("Source File Found", "[Core][Source]") {
    auto result = VCL::Source::LoadFromDisk("VCL/empty.vcl");
    REQUIRE(result.has_value() == true);
}

TEST_CASE("Source File Not Found", "[Core][Source]") {
    auto result = VCL::Source::LoadFromDisk("BigNoNoPath");
    REQUIRE(result.has_value() == false);
    REQUIRE(result.error() == VCL::SourceError::FileNotFound);
}