#include <catch2/catch_test_macros.hpp>

#include <catch2/generators/catch_generators_random.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>

#include <VCL/Core/SourceManager.hpp>
#include <VCL/Core/Source.hpp>
#include <VCL/Frontend/CompilerContext.hpp>
#include <VCL/Frontend/CompilerInstance.hpp>
#include <VCL/Frontend/FrontendActions.hpp>
#include <VCL/Frontend/ExecutionSession.hpp>

#include "../Common/ExpectedDiagnostic.hpp"
#include "../Common/MakeModule.hpp"


TEST_CASE("Builtin Passthrough", "[Frontend]") {
    VCL::ExecutionSession session{};
    session.EnableGDBListener();
    REQUIRE(session.SubmitModule(MakeModule("VCL/builtinpassthrough.vcl")));

    SECTION("Value Check") {
        float if32 = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(std::numeric_limits<float>::min(), std::numeric_limits<float>::max())));
        double if64 = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(std::numeric_limits<double>::min(), std::numeric_limits<double>::max())));

        int8_t ii8 = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max())));
        int16_t ii16 = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max())));
        int32_t ii32 = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max())));
        int64_t ii64 = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max())));

        uint8_t iu8 = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(std::numeric_limits<uint8_t>::min(), std::numeric_limits<uint8_t>::max())));
        uint16_t iu16 = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(std::numeric_limits<uint16_t>::min(), std::numeric_limits<uint16_t>::max())));
        uint32_t iu32 = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(std::numeric_limits<uint32_t>::min(), std::numeric_limits<uint32_t>::max())));
        uint64_t iu64 = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(std::numeric_limits<uint64_t>::min(), std::numeric_limits<uint64_t>::max())));

        bool ib = GENERATE(true, false);

        REQUIRE(session.DefineSymbolPtr("if32", &if32));
        REQUIRE(session.DefineSymbolPtr("if64", &if64));

        REQUIRE(session.DefineSymbolPtr("ii8", &ii8));
        REQUIRE(session.DefineSymbolPtr("ii16", &ii16));
        REQUIRE(session.DefineSymbolPtr("ii32", &ii32));
        REQUIRE(session.DefineSymbolPtr("ii64", &ii64));

        REQUIRE(session.DefineSymbolPtr("iu8", &iu8));
        REQUIRE(session.DefineSymbolPtr("iu16", &iu16));
        REQUIRE(session.DefineSymbolPtr("iu32", &iu32));
        REQUIRE(session.DefineSymbolPtr("iu64", &iu64));

        REQUIRE(session.DefineSymbolPtr("ib", &ib));

        float* of32 = (float*)session.Lookup("of32");
        double* of64 = (double*)session.Lookup("of64");

        int8_t* oi8 = (int8_t*)session.Lookup("oi8");
        int16_t* oi16 = (int16_t*)session.Lookup("oi16");
        int32_t* oi32 = (int32_t*)session.Lookup("oi32");
        int64_t* oi64 = (int64_t*)session.Lookup("oi64");

        uint8_t* ou8 = (uint8_t*)session.Lookup("ou8");
        uint16_t* ou16 = (uint16_t*)session.Lookup("ou16");
        uint32_t* ou32 = (uint32_t*)session.Lookup("ou32");
        uint64_t* ou64 = (uint64_t*)session.Lookup("ou64");

        bool* ob = (bool*)session.Lookup("ob");

        REQUIRE(of32 != nullptr);
        REQUIRE(of64 != nullptr);

        REQUIRE(oi8 != nullptr);
        REQUIRE(oi16 != nullptr);
        REQUIRE(oi32 != nullptr);
        REQUIRE(oi64 != nullptr);

        REQUIRE(ou8 != nullptr);
        REQUIRE(ou16 != nullptr);
        REQUIRE(ou32 != nullptr);
        REQUIRE(ou64 != nullptr);

        REQUIRE(ob != nullptr);

        void* main = session.Lookup("Main");
        if (!main) {
            INFO(llvm::toString(session.ConsumeLastError()));
            REQUIRE(false);
        }

        ((void(*)())main)();

        REQUIRE(if32 == *of32);
        REQUIRE(if64 == *of64);

        REQUIRE(ii8 == *oi8);
        REQUIRE(ii16 == *oi16);
        REQUIRE(ii32 == *oi32);
        REQUIRE(ii64 == *oi64);

        REQUIRE(iu8 == *ou8);
        REQUIRE(iu16 == *ou16);
        REQUIRE(iu32 == *ou32);
        REQUIRE(iu64 == *ou64);

        REQUIRE(ib == *ob);
    }
}

// D1 (Grog docs/vcl-review.md): an aggregate parameter without a direction qualifier is a
// read-only reference; `inout` (or `out`) makes it writable.
TEST_CASE("Aggregate Parameters Are Read-Only By Default", "[Frontend]") {
    SECTION("Reading, and writing through inout") {
        VCL::ExecutionSession session{};
        REQUIRE(session.SubmitModule(MakeModuleFromSource(
            "out float32 o_sum;\n"
            "out float32 o_after;\n"
            "out float32 o_first;\n"
            "out float32 o_table;\n"
            "const Array<float32, 2> table = { 7.0, 8.0 };\n"
            "struct S { float32 v; }\n"
            "float32 Sum(Array<float32, 4> a) { return a[0] + a[1] + a[2] + a[3]; }\n"
            "float32 Second(Array<float32, 2> a) { return a[1]; }\n"
            "void Fill(inout Array<float32, 4> a) { a[1] = 10.0; }\n"
            "void Bump(inout S s) { s.v = s.v + 1.0; }\n"
            "template<typename T> float32 First(T a) { return a[0]; }\n"
            "[EntryPoint] void Main() {\n"
            "    Array<float32, 4> arr = { 1.0, 2.0, 3.0, 4.0 };\n"
            "    o_sum = Sum(arr);\n"
            "    Fill(arr);\n"
            "    S s = { 5.0 };\n"
            "    Bump(s);\n"
            "    o_after = arr[1] + s.v;\n"
            "    o_first = First(arr);\n"
            "    o_table = Second(table);\n"
            "}\n")));

        auto* o_sum = (float*)session.Lookup("o_sum");
        auto* o_after = (float*)session.Lookup("o_after");
        auto* o_first = (float*)session.Lookup("o_first");
        auto* o_table = (float*)session.Lookup("o_table");
        void* main = session.Lookup("Main");
        REQUIRE(main != nullptr);
        ((void(*)())main)();

        REQUIRE(*o_sum == 10.0f);
        REQUIRE(*o_after == 16.0f);
        REQUIRE(*o_first == 1.0f);
        REQUIRE(*o_table == 8.0f); // a const array can be passed to a default parameter
    }
    SECTION("Writing an array element is an error") {
        RequireDiagnosticFromSource<VCL::Diagnostic::AssignmentConstValue>(
            "void F(Array<float32, 4> a) { a[0] = 1.0; }");
    }
    SECTION("Writing a field is an error") {
        RequireDiagnosticFromSource<VCL::Diagnostic::AssignmentConstValue>(
            "struct S { float32 v; } void F(S s) { s.v = 1.0; }");
    }
    SECTION("Passing it on to an inout parameter is an error") {
        RequireDiagnosticFromSource<VCL::Diagnostic::QualifierDropped>(
            "void G(inout Array<float32, 4> a); void F(Array<float32, 4> a) { G(a); }");
    }
    SECTION("Passing an element on to an out parameter is an error") {
        RequireDiagnosticFromSource<VCL::Diagnostic::QualifierDropped>(
            "void G(out float32 v); void F(Array<float32, 4> a) { G(a[0]); }");
    }
    SECTION("A dependent parameter that becomes an aggregate") {
        RequireDiagnosticFromSource<VCL::Diagnostic::AssignmentConstValue>(
            "template<typename T> void F(T a) { a[0] = 1.0; }"
            "[EntryPoint] void Main() { Array<float32, 4> arr; F(arr); }");
    }
}