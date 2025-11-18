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


TEST_CASE("Numeric Cast", "[Frontend]") {
    VCL::ExecutionSession session{};
    session.EnableGDBListener();
    REQUIRE(session.SubmitModule(MakeModule("VCL/numericcast.vcl")));

    SECTION("Value Check") {
        // Generate random input values for testing
        float if32 = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(-1000.0f, 1000.0f)));
        double if64 = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(-1000.0, 1000.0)));
        int8_t ii8 = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(static_cast<int8_t>(-100), static_cast<int8_t>(100))));
        int16_t ii16 = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(static_cast<int16_t>(-1000), static_cast<int16_t>(1000))));
        int32_t ii32 = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(-100000, 100000)));
        int64_t ii64 = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(static_cast<int64_t>(-100000), static_cast<int64_t>(100000))));
        uint8_t iu8 = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(static_cast<uint8_t>(0), static_cast<uint8_t>(200))));
        uint16_t iu16 = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(static_cast<uint16_t>(0), static_cast<uint16_t>(2000))));
        uint32_t iu32 = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(static_cast<uint32_t>(0), static_cast<uint32_t>(200000))));
        uint64_t iu64 = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(static_cast<uint64_t>(0), static_cast<uint64_t>(200000))));

        // Define input symbols
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

        // Lookup output symbols - Floating point casts
        double* o_fce_f32 = (double*)session.Lookup("o_fce_f32");
        float* o_fct_f64 = (float*)session.Lookup("o_fct_f64");

        // Floating to signed integer casts
        int8_t* o_fts_f32_i8 = (int8_t*)session.Lookup("o_fts_f32_i8");
        int16_t* o_fts_f32_i16 = (int16_t*)session.Lookup("o_fts_f32_i16");
        int32_t* o_fts_f32_i32 = (int32_t*)session.Lookup("o_fts_f32_i32");
        int64_t* o_fts_f32_i64 = (int64_t*)session.Lookup("o_fts_f32_i64");
        int64_t* o_fts_f64_i64 = (int64_t*)session.Lookup("o_fts_f64_i64");

        // Floating to unsigned integer casts
        uint8_t* o_ftu_f32_u8 = (uint8_t*)session.Lookup("o_ftu_f32_u8");
        uint16_t* o_ftu_f32_u16 = (uint16_t*)session.Lookup("o_ftu_f32_u16");
        uint32_t* o_ftu_f32_u32 = (uint32_t*)session.Lookup("o_ftu_f32_u32");
        uint64_t* o_ftu_f32_u64 = (uint64_t*)session.Lookup("o_ftu_f32_u64");
        uint64_t* o_ftu_f64_u64 = (uint64_t*)session.Lookup("o_ftu_f64_u64");

        // Signed integer extension casts
        int16_t* o_sce_i8_i16 = (int16_t*)session.Lookup("o_sce_i8_i16");
        int32_t* o_sce_i8_i32 = (int32_t*)session.Lookup("o_sce_i8_i32");
        int64_t* o_sce_i8_i64 = (int64_t*)session.Lookup("o_sce_i8_i64");
        int32_t* o_sce_i16_i32 = (int32_t*)session.Lookup("o_sce_i16_i32");
        int64_t* o_sce_i16_i64 = (int64_t*)session.Lookup("o_sce_i16_i64");
        int64_t* o_sce_i32_i64 = (int64_t*)session.Lookup("o_sce_i32_i64");

        // Signed integer truncation casts
        int8_t* o_sct_i16_i8 = (int8_t*)session.Lookup("o_sct_i16_i8");
        int8_t* o_sct_i32_i8 = (int8_t*)session.Lookup("o_sct_i32_i8");
        int8_t* o_sct_i64_i8 = (int8_t*)session.Lookup("o_sct_i64_i8");
        int16_t* o_sct_i32_i16 = (int16_t*)session.Lookup("o_sct_i32_i16");
        int16_t* o_sct_i64_i16 = (int16_t*)session.Lookup("o_sct_i64_i16");
        int32_t* o_sct_i64_i32 = (int32_t*)session.Lookup("o_sct_i64_i32");

        // Signed to floating casts
        float* o_stf_i8_f32 = (float*)session.Lookup("o_stf_i8_f32");
        float* o_stf_i16_f32 = (float*)session.Lookup("o_stf_i16_f32");
        float* o_stf_i32_f32 = (float*)session.Lookup("o_stf_i32_f32");
        double* o_stf_i64_f64 = (double*)session.Lookup("o_stf_i64_f64");

        // Signed to unsigned casts
        uint8_t* o_stu_i8_u8 = (uint8_t*)session.Lookup("o_stu_i8_u8");
        uint16_t* o_stu_i16_u16 = (uint16_t*)session.Lookup("o_stu_i16_u16");
        uint32_t* o_stu_i32_u32 = (uint32_t*)session.Lookup("o_stu_i32_u32");
        uint64_t* o_stu_i64_u64 = (uint64_t*)session.Lookup("o_stu_i64_u64");

        // Unsigned integer extension casts
        uint16_t* o_uce_u8_u16 = (uint16_t*)session.Lookup("o_uce_u8_u16");
        uint32_t* o_uce_u8_u32 = (uint32_t*)session.Lookup("o_uce_u8_u32");
        uint64_t* o_uce_u8_u64 = (uint64_t*)session.Lookup("o_uce_u8_u64");
        uint32_t* o_uce_u16_u32 = (uint32_t*)session.Lookup("o_uce_u16_u32");
        uint64_t* o_uce_u16_u64 = (uint64_t*)session.Lookup("o_uce_u16_u64");
        uint64_t* o_uce_u32_u64 = (uint64_t*)session.Lookup("o_uce_u32_u64");

        // Unsigned integer truncation casts
        uint8_t* o_uct_u16_u8 = (uint8_t*)session.Lookup("o_uct_u16_u8");
        uint8_t* o_uct_u32_u8 = (uint8_t*)session.Lookup("o_uct_u32_u8");
        uint8_t* o_uct_u64_u8 = (uint8_t*)session.Lookup("o_uct_u64_u8");
        uint16_t* o_uct_u32_u16 = (uint16_t*)session.Lookup("o_uct_u32_u16");
        uint16_t* o_uct_u64_u16 = (uint16_t*)session.Lookup("o_uct_u64_u16");
        uint32_t* o_uct_u64_u32 = (uint32_t*)session.Lookup("o_uct_u64_u32");

        // Unsigned to floating casts
        float* o_utf_u8_f32 = (float*)session.Lookup("o_utf_u8_f32");
        float* o_utf_u16_f32 = (float*)session.Lookup("o_utf_u16_f32");
        float* o_utf_u32_f32 = (float*)session.Lookup("o_utf_u32_f32");
        double* o_utf_u64_f64 = (double*)session.Lookup("o_utf_u64_f64");

        // Unsigned to signed casts
        int8_t* o_uts_u8_i8 = (int8_t*)session.Lookup("o_uts_u8_i8");
        int16_t* o_uts_u16_i16 = (int16_t*)session.Lookup("o_uts_u16_i16");
        int32_t* o_uts_u32_i32 = (int32_t*)session.Lookup("o_uts_u32_i32");
        int64_t* o_uts_u64_i64 = (int64_t*)session.Lookup("o_uts_u64_i64");

        // Verify all pointers are valid
        REQUIRE(o_fce_f32 != nullptr);
        REQUIRE(o_fct_f64 != nullptr);
        REQUIRE(o_fts_f32_i8 != nullptr);
        REQUIRE(o_fts_f32_i16 != nullptr);
        REQUIRE(o_fts_f32_i32 != nullptr);
        REQUIRE(o_fts_f32_i64 != nullptr);
        REQUIRE(o_fts_f64_i64 != nullptr);
        REQUIRE(o_ftu_f32_u8 != nullptr);
        REQUIRE(o_ftu_f32_u16 != nullptr);
        REQUIRE(o_ftu_f32_u32 != nullptr);
        REQUIRE(o_ftu_f32_u64 != nullptr);
        REQUIRE(o_ftu_f64_u64 != nullptr);
        REQUIRE(o_sce_i8_i16 != nullptr);
        REQUIRE(o_sce_i8_i32 != nullptr);
        REQUIRE(o_sce_i8_i64 != nullptr);
        REQUIRE(o_sce_i16_i32 != nullptr);
        REQUIRE(o_sce_i16_i64 != nullptr);
        REQUIRE(o_sce_i32_i64 != nullptr);
        REQUIRE(o_sct_i16_i8 != nullptr);
        REQUIRE(o_sct_i32_i8 != nullptr);
        REQUIRE(o_sct_i64_i8 != nullptr);
        REQUIRE(o_sct_i32_i16 != nullptr);
        REQUIRE(o_sct_i64_i16 != nullptr);
        REQUIRE(o_sct_i64_i32 != nullptr);
        REQUIRE(o_stf_i8_f32 != nullptr);
        REQUIRE(o_stf_i16_f32 != nullptr);
        REQUIRE(o_stf_i32_f32 != nullptr);
        REQUIRE(o_stf_i64_f64 != nullptr);
        REQUIRE(o_stu_i8_u8 != nullptr);
        REQUIRE(o_stu_i16_u16 != nullptr);
        REQUIRE(o_stu_i32_u32 != nullptr);
        REQUIRE(o_stu_i64_u64 != nullptr);
        REQUIRE(o_uce_u8_u16 != nullptr);
        REQUIRE(o_uce_u8_u32 != nullptr);
        REQUIRE(o_uce_u8_u64 != nullptr);
        REQUIRE(o_uce_u16_u32 != nullptr);
        REQUIRE(o_uce_u16_u64 != nullptr);
        REQUIRE(o_uce_u32_u64 != nullptr);
        REQUIRE(o_uct_u16_u8 != nullptr);
        REQUIRE(o_uct_u32_u8 != nullptr);
        REQUIRE(o_uct_u64_u8 != nullptr);
        REQUIRE(o_uct_u32_u16 != nullptr);
        REQUIRE(o_uct_u64_u16 != nullptr);
        REQUIRE(o_uct_u64_u32 != nullptr);
        REQUIRE(o_utf_u8_f32 != nullptr);
        REQUIRE(o_utf_u16_f32 != nullptr);
        REQUIRE(o_utf_u32_f32 != nullptr);
        REQUIRE(o_utf_u64_f64 != nullptr);
        REQUIRE(o_uts_u8_i8 != nullptr);
        REQUIRE(o_uts_u16_i16 != nullptr);
        REQUIRE(o_uts_u32_i32 != nullptr);
        REQUIRE(o_uts_u64_i64 != nullptr);

        // Execute the Main function
        void* main = session.Lookup("Main");
        if (!main) {
            INFO(llvm::toString(session.ConsumeLastError()));
            REQUIRE(false);
        }
        ((void(*)())main)();

        // Verify floating point casts
        REQUIRE(*o_fce_f32 == static_cast<double>(if32));     // FloatingCastExt
        REQUIRE(*o_fct_f64 == static_cast<float>(if64));      // FloatingCastTrunc

        // Verify floating to signed integer casts
        REQUIRE(*o_fts_f32_i8 == static_cast<int8_t>(if32));    // FloatingToSigned
        REQUIRE(*o_fts_f32_i16 == static_cast<int16_t>(if32));  // FloatingToSigned
        REQUIRE(*o_fts_f32_i32 == static_cast<int32_t>(if32));  // FloatingToSigned
        REQUIRE(*o_fts_f32_i64 == static_cast<int64_t>(if32));  // FloatingToSigned
        REQUIRE(*o_fts_f64_i64 == static_cast<int64_t>(if64));  // FloatingToSigned

        // Verify floating to unsigned integer casts
        REQUIRE(*o_ftu_f32_u8 == static_cast<uint8_t>(if32));   // FloatingToUnsigned
        REQUIRE(*o_ftu_f32_u16 == static_cast<uint16_t>(if32)); // FloatingToUnsigned
        REQUIRE(*o_ftu_f32_u32 == static_cast<uint32_t>(if32)); // FloatingToUnsigned
        REQUIRE(*o_ftu_f32_u64 == static_cast<uint64_t>(if32)); // FloatingToUnsigned
        REQUIRE(*o_ftu_f64_u64 == static_cast<uint64_t>(if64)); // FloatingToUnsigned
        
        // Verify signed integer extension casts
        REQUIRE(*o_sce_i8_i16 == static_cast<int16_t>(ii8));    // SignedCastExt
        REQUIRE(*o_sce_i8_i32 == static_cast<int32_t>(ii8));    // SignedCastExt
        REQUIRE(*o_sce_i8_i64 == static_cast<int64_t>(ii8));    // SignedCastExt
        REQUIRE(*o_sce_i16_i32 == static_cast<int32_t>(ii16));  // SignedCastExt
        REQUIRE(*o_sce_i16_i64 == static_cast<int64_t>(ii16));  // SignedCastExt
        REQUIRE(*o_sce_i32_i64 == static_cast<int64_t>(ii32));  // SignedCastExt

        // Verify signed integer truncation casts
        REQUIRE(*o_sct_i16_i8 == static_cast<int8_t>(ii16));    // SignedCastTrunc
        REQUIRE(*o_sct_i32_i8 == static_cast<int8_t>(ii32));    // SignedCastTrunc
        REQUIRE(*o_sct_i64_i8 == static_cast<int8_t>(ii64));    // SignedCastTrunc
        REQUIRE(*o_sct_i32_i16 == static_cast<int16_t>(ii32));  // SignedCastTrunc
        REQUIRE(*o_sct_i64_i16 == static_cast<int16_t>(ii64));  // SignedCastTrunc
        REQUIRE(*o_sct_i64_i32 == static_cast<int32_t>(ii64));  // SignedCastTrunc

        // Verify signed to floating casts
        REQUIRE(*o_stf_i8_f32 == static_cast<float>(ii8));      // SignedToFloating
        REQUIRE(*o_stf_i16_f32 == static_cast<float>(ii16));    // SignedToFloating
        REQUIRE(*o_stf_i32_f32 == static_cast<float>(ii32));    // SignedToFloating
        REQUIRE(*o_stf_i64_f64 == static_cast<double>(ii64));   // SignedToFloating

        // Verify signed to unsigned casts
        REQUIRE(*o_stu_i8_u8 == static_cast<uint8_t>(ii8));     // SignedToUnsigned
        REQUIRE(*o_stu_i16_u16 == static_cast<uint16_t>(ii16)); // SignedToUnsigned
        REQUIRE(*o_stu_i32_u32 == static_cast<uint32_t>(ii32)); // SignedToUnsigned
        REQUIRE(*o_stu_i64_u64 == static_cast<uint64_t>(ii64)); // SignedToUnsigned

        // Verify unsigned integer extension casts
        REQUIRE(*o_uce_u8_u16 == static_cast<uint16_t>(iu8));   // UnsignedCastExt
        REQUIRE(*o_uce_u8_u32 == static_cast<uint32_t>(iu8));   // UnsignedCastExt
        REQUIRE(*o_uce_u8_u64 == static_cast<uint64_t>(iu8));   // UnsignedCastExt
        REQUIRE(*o_uce_u16_u32 == static_cast<uint32_t>(iu16)); // UnsignedCastExt
        REQUIRE(*o_uce_u16_u64 == static_cast<uint64_t>(iu16)); // UnsignedCastExt
        REQUIRE(*o_uce_u32_u64 == static_cast<uint64_t>(iu32)); // UnsignedCastExt

        // Verify unsigned integer truncation casts
        REQUIRE(*o_uct_u16_u8 == static_cast<uint8_t>(iu16));   // UnsignedCastTrunc
        REQUIRE(*o_uct_u32_u8 == static_cast<uint8_t>(iu32));   // UnsignedCastTrunc
        REQUIRE(*o_uct_u64_u8 == static_cast<uint8_t>(iu64));   // UnsignedCastTrunc
        REQUIRE(*o_uct_u32_u16 == static_cast<uint16_t>(iu32)); // UnsignedCastTrunc
        REQUIRE(*o_uct_u64_u16 == static_cast<uint16_t>(iu64)); // UnsignedCastTrunc
        REQUIRE(*o_uct_u64_u32 == static_cast<uint32_t>(iu64)); // UnsignedCastTrunc

        // Verify unsigned to floating casts
        REQUIRE(*o_utf_u8_f32 == static_cast<float>(iu8));      // UnsignedToFloating
        REQUIRE(*o_utf_u16_f32 == static_cast<float>(iu16));    // UnsignedToFloating
        REQUIRE(*o_utf_u32_f32 == static_cast<float>(iu32));    // UnsignedToFloating
        REQUIRE(*o_utf_u64_f64 == static_cast<double>(iu64));   // UnsignedToFloating

        // Verify unsigned to signed casts
        REQUIRE(*o_uts_u8_i8 == static_cast<int8_t>(iu8));      // UnsignedToSigned
        REQUIRE(*o_uts_u16_i16 == static_cast<int16_t>(iu16));  // UnsignedToSigned
        REQUIRE(*o_uts_u32_i32 == static_cast<int32_t>(iu32));  // UnsignedToSigned
        REQUIRE(*o_uts_u64_i64 == static_cast<int64_t>(iu64));  // UnsignedToSigned
    }
}

TEST_CASE("Binary Expressions", "[Frontend]") {
    VCL::ExecutionSession session{};
    session.EnableGDBListener();
    REQUIRE(session.SubmitModule(MakeModule("VCL/binaryexpr.vcl")));

    SECTION("Value Check") {
        // Generate random input values for testing
        float a = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(-100.0f, 100.0f)));
        float b = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(-100.0f, 100.0f)));
        int32_t ia = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(-1000, 1000)));
        int32_t ib = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(1, 1000))); // Avoid division by zero

        // Define input symbols
        REQUIRE(session.DefineSymbolPtr("a", &a));
        REQUIRE(session.DefineSymbolPtr("b", &b));
        REQUIRE(session.DefineSymbolPtr("ia", &ia));
        REQUIRE(session.DefineSymbolPtr("ib", &ib));

        // Lookup output symbols
        float* o_add = (float*)session.Lookup("o_add");
        float* o_sub = (float*)session.Lookup("o_sub");
        float* o_mul = (float*)session.Lookup("o_mul");
        float* o_div = (float*)session.Lookup("o_div");
        int32_t* o_iadd = (int32_t*)session.Lookup("o_iadd");
        int32_t* o_isub = (int32_t*)session.Lookup("o_isub");
        int32_t* o_imul = (int32_t*)session.Lookup("o_imul");
        int32_t* o_idiv = (int32_t*)session.Lookup("o_idiv");
        int32_t* o_irem = (int32_t*)session.Lookup("o_irem");
        float* o_assign = (float*)session.Lookup("o_assign");
        float* o_add_assign = (float*)session.Lookup("o_add_assign");
        float* o_sub_assign = (float*)session.Lookup("o_sub_assign");
        float* o_mul_assign = (float*)session.Lookup("o_mul_assign");
        float* o_div_assign = (float*)session.Lookup("o_div_assign");

        // Verify all pointers are valid
        REQUIRE(o_add != nullptr);
        REQUIRE(o_sub != nullptr);
        REQUIRE(o_mul != nullptr);
        REQUIRE(o_div != nullptr);
        REQUIRE(o_iadd != nullptr);
        REQUIRE(o_isub != nullptr);
        REQUIRE(o_imul != nullptr);
        REQUIRE(o_idiv != nullptr);
        REQUIRE(o_irem != nullptr);
        REQUIRE(o_assign != nullptr);
        REQUIRE(o_add_assign != nullptr);
        REQUIRE(o_sub_assign != nullptr);
        REQUIRE(o_mul_assign != nullptr);
        REQUIRE(o_div_assign != nullptr);

        // Execute the Main function
        void* main = session.Lookup("Main");
        REQUIRE(main != nullptr);
        ((void(*)())main)();

        // Verify arithmetic operations
        REQUIRE(*o_add == (a + b));
        REQUIRE(*o_sub == (a - b));
        REQUIRE(*o_mul == (a * b));
        REQUIRE(*o_div == (a / b));
        
        // Verify integer arithmetic
        REQUIRE(*o_iadd == (ia + ib));
        REQUIRE(*o_isub == (ia - ib));
        REQUIRE(*o_imul == (ia * ib));
        REQUIRE(*o_idiv == (ia / ib));
        REQUIRE(*o_irem == (ia % ib));
        
        // Verify assignment operations
        REQUIRE(*o_assign == a);
        REQUIRE(*o_add_assign == (a + b));
        REQUIRE(*o_sub_assign == (a - b));
        REQUIRE(*o_mul_assign == (a * b));
        REQUIRE(*o_div_assign == (a / b));
    }
}

TEST_CASE("Function Call Expressions", "[Frontend]") {
    VCL::ExecutionSession session{};
    session.EnableGDBListener();
    REQUIRE(session.SubmitModule(MakeModule("VCL/callexpr.vcl")));

    SECTION("Value Check") {
        // Generate random input values for testing
        float a = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(-50.0f, 50.0f)));
        float b = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(-50.0f, 50.0f)));
        int32_t ia = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(-100, 100)));
        int32_t ib = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(-100, 100)));

        // Define input symbols
        REQUIRE(session.DefineSymbolPtr("a", &a));
        REQUIRE(session.DefineSymbolPtr("b", &b));
        REQUIRE(session.DefineSymbolPtr("ia", &ia));
        REQUIRE(session.DefineSymbolPtr("ib", &ib));

        // Lookup output symbols
        float* o_add_func = (float*)session.Lookup("o_add_func");
        float* o_mul_func = (float*)session.Lookup("o_mul_func");
        int32_t* o_abs_func = (int32_t*)session.Lookup("o_abs_func");
        float* o_lerp_func = (float*)session.Lookup("o_lerp_func");
        float* o_nested_func = (float*)session.Lookup("o_nested_func");

        // Verify all pointers are valid
        REQUIRE(o_add_func != nullptr);
        REQUIRE(o_mul_func != nullptr);
        REQUIRE(o_abs_func != nullptr);
        REQUIRE(o_lerp_func != nullptr);
        REQUIRE(o_nested_func != nullptr);

        // Execute the Main function
        void* main = session.Lookup("Main");
        REQUIRE(main != nullptr);
        ((void(*)())main)();

        // Verify function call results
        REQUIRE(*o_add_func == (a + b));
        REQUIRE(*o_mul_func == (a * b));
        REQUIRE(*o_abs_func == (ia * ((ia > 0) - (ia < 0))));  // Branchless abs formula
        REQUIRE(*o_lerp_func == (a + (b - a) * 0.5f));  // Lerp formula
        
        // Verify nested function call (ComplexCalc = Add(Add(a,b), Multiply(a,b)))
        float expected_nested = (a + b) + (a * b);
        REQUIRE(*o_nested_func == expected_nested);
    }
}

TEST_CASE("Field Access Expressions", "[Frontend]") {
    VCL::ExecutionSession session{};
    session.EnableGDBListener();
    REQUIRE(session.SubmitModule(MakeModule("VCL/fieldaccess.vcl")));

    SECTION("Value Check") {
        // Generate random input values for testing
        float input_x = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(-100.0f, 100.0f)));
        float input_y = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(-100.0f, 100.0f)));
        int32_t input_count = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(1, 1000)));

        // Define input symbols
        REQUIRE(session.DefineSymbolPtr("input_x", &input_x));
        REQUIRE(session.DefineSymbolPtr("input_y", &input_y));
        REQUIRE(session.DefineSymbolPtr("input_count", &input_count));

        // Lookup output symbols
        float* o_point_x = (float*)session.Lookup("o_point_x");
        float* o_point_y = (float*)session.Lookup("o_point_y");
        float* o_rect_x = (float*)session.Lookup("o_rect_x");
        float* o_rect_y = (float*)session.Lookup("o_rect_y");
        int32_t* o_rect_id = (int32_t*)session.Lookup("o_rect_id");
        float* o_complex_x = (float*)session.Lookup("o_complex_x");
        float* o_scale = (float*)session.Lookup("o_scale");
        int32_t* o_flags = (int32_t*)session.Lookup("o_flags");
        float* o_modified_x = (float*)session.Lookup("o_modified_x");
        float* o_modified_y = (float*)session.Lookup("o_modified_y");

        // Verify all pointers are valid
        REQUIRE(o_point_x != nullptr);
        REQUIRE(o_point_y != nullptr);
        REQUIRE(o_rect_x != nullptr);
        REQUIRE(o_rect_y != nullptr);
        REQUIRE(o_rect_id != nullptr);
        REQUIRE(o_complex_x != nullptr);
        REQUIRE(o_scale != nullptr);
        REQUIRE(o_flags != nullptr);
        REQUIRE(o_modified_x != nullptr);
        REQUIRE(o_modified_y != nullptr);

        // Execute the Main function
        void* main = session.Lookup("Main");
        REQUIRE(main != nullptr);
        ((void(*)())main)();

        // Verify field access results
        REQUIRE(*o_point_x == input_x);
        REQUIRE(*o_point_y == input_y);
        REQUIRE(*o_rect_x == input_x);
        REQUIRE(*o_rect_y == input_y);
        REQUIRE(*o_rect_id == input_count);
        REQUIRE(*o_complex_x == input_x);
        REQUIRE(*o_scale == (input_x * 2.0f));
        REQUIRE(*o_flags == (input_count * 2));
        
        // Verify field modification
        REQUIRE(*o_modified_x == (input_x * 3.0f));
        REQUIRE(*o_modified_y == (input_y + 5.0f));
    }
}