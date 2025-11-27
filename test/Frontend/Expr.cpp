#include <catch2/catch_test_macros.hpp>

#include <catch2/generators/catch_generators_random.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>

#include <VCL/Core/SourceManager.hpp>
#include <VCL/Core/Source.hpp>
#include <VCL/Core/Target.hpp>
#include <VCL/Frontend/CompilerContext.hpp>
#include <VCL/Frontend/CompilerInstance.hpp>
#include <VCL/Frontend/FrontendActions.hpp>
#include <VCL/Frontend/ExecutionSession.hpp>
#include <VCL/Frontend/Storage.hpp>

#include "../Common/ExpectedDiagnostic.hpp"
#include "../Common/MakeModule.hpp"

#include <format>


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
        uint32_t ua = GENERATE(Catch::Generators::take(1,
                Catch::Generators::random(static_cast<uint32_t>(0), static_cast<uint32_t>(2000))));
        uint32_t ub = GENERATE(Catch::Generators::take(1,
                Catch::Generators::random(static_cast<uint32_t>(1), static_cast<uint32_t>(2000)))); // Avoid division by zero

        // Define input symbols
        REQUIRE(session.DefineSymbolPtr("a", &a));
        REQUIRE(session.DefineSymbolPtr("b", &b));
        REQUIRE(session.DefineSymbolPtr("ia", &ia));
        REQUIRE(session.DefineSymbolPtr("ib", &ib));
        REQUIRE(session.DefineSymbolPtr("ua", &ua));
        REQUIRE(session.DefineSymbolPtr("ub", &ub));

        // Lookup float arithmetic output symbols
        float* o_add = (float*)session.Lookup("o_add");
        float* o_sub = (float*)session.Lookup("o_sub");
        float* o_mul = (float*)session.Lookup("o_mul");
        float* o_div = (float*)session.Lookup("o_div");

        // Lookup float comparison output symbols
        bool* o_fgreater = (bool*)session.Lookup("o_fgreater");
        bool* o_flesser = (bool*)session.Lookup("o_flesser");
        bool* o_fgreater_eq = (bool*)session.Lookup("o_fgreater_eq");
        bool* o_flesser_eq = (bool*)session.Lookup("o_flesser_eq");
        bool* o_fequal = (bool*)session.Lookup("o_fequal");
        bool* o_fnot_equal = (bool*)session.Lookup("o_fnot_equal");

        // Lookup integer arithmetic output symbols
        int32_t* o_iadd = (int32_t*)session.Lookup("o_iadd");
        int32_t* o_isub = (int32_t*)session.Lookup("o_isub");
        int32_t* o_imul = (int32_t*)session.Lookup("o_imul");
        int32_t* o_idiv = (int32_t*)session.Lookup("o_idiv");
        int32_t* o_irem = (int32_t*)session.Lookup("o_irem");

        // Lookup integer bitwise output symbols
        int32_t* o_band = (int32_t*)session.Lookup("o_band");
        int32_t* o_bor = (int32_t*)session.Lookup("o_bor");
        int32_t* o_bxor = (int32_t*)session.Lookup("o_bxor");
        int32_t* o_lshift = (int32_t*)session.Lookup("o_lshift");
        int32_t* o_rshift = (int32_t*)session.Lookup("o_rshift");

        // Lookup integer comparison output symbols
        bool* o_greater = (bool*)session.Lookup("o_greater");
        bool* o_lesser = (bool*)session.Lookup("o_lesser");
        bool* o_greater_eq = (bool*)session.Lookup("o_greater_eq");
        bool* o_lesser_eq = (bool*)session.Lookup("o_lesser_eq");
        bool* o_equal = (bool*)session.Lookup("o_equal");
        bool* o_not_equal = (bool*)session.Lookup("o_not_equal");

        // Lookup unsigned arithmetic output symbols
        uint32_t* o_uadd = (uint32_t*)session.Lookup("o_uadd");
        uint32_t* o_usub = (uint32_t*)session.Lookup("o_usub");
        uint32_t* o_umul = (uint32_t*)session.Lookup("o_umul");
        uint32_t* o_udiv = (uint32_t*)session.Lookup("o_udiv");
        uint32_t* o_urem = (uint32_t*)session.Lookup("o_urem");

        // Lookup unsigned bitwise output symbols
        uint32_t* o_uband = (uint32_t*)session.Lookup("o_uband");
        uint32_t* o_ubor = (uint32_t*)session.Lookup("o_ubor");
        uint32_t* o_ubxor = (uint32_t*)session.Lookup("o_ubxor");
        uint32_t* o_ulshift = (uint32_t*)session.Lookup("o_ulshift");
        uint32_t* o_urshift = (uint32_t*)session.Lookup("o_urshift");

        // Lookup unsigned comparison output symbols
        bool* o_ugreater = (bool*)session.Lookup("o_ugreater");
        bool* o_uresser = (bool*)session.Lookup("o_uresser");
        bool* o_ugreater_eq = (bool*)session.Lookup("o_ugreater_eq");
        bool* o_uresser_eq = (bool*)session.Lookup("o_uresser_eq");
        bool* o_uequal = (bool*)session.Lookup("o_uequal");
        bool* o_unot_equal = (bool*)session.Lookup("o_unot_equal");

        // Lookup logical output symbols
        bool* o_land = (bool*)session.Lookup("o_land");
        bool* o_lor = (bool*)session.Lookup("o_lor");

        // Lookup float assignment output symbols
        float* o_assign = (float*)session.Lookup("o_assign");
        float* o_add_assign = (float*)session.Lookup("o_add_assign");
        float* o_sub_assign = (float*)session.Lookup("o_sub_assign");
        float* o_mul_assign = (float*)session.Lookup("o_mul_assign");
        float* o_div_assign = (float*)session.Lookup("o_div_assign");

        // Lookup integer assignment output symbols
        int32_t* o_rem_assign = (int32_t*)session.Lookup("o_rem_assign");
        int32_t* o_band_assign = (int32_t*)session.Lookup("o_band_assign");
        int32_t* o_bor_assign = (int32_t*)session.Lookup("o_bor_assign");
        int32_t* o_bxor_assign = (int32_t*)session.Lookup("o_bxor_assign");
        int32_t* o_lshift_assign = (int32_t*)session.Lookup("o_lshift_assign");
        int32_t* o_rshift_assign = (int32_t*)session.Lookup("o_rshift_assign");

        // Lookup unsigned assignment output symbols
        uint32_t* o_uadd_assign = (uint32_t*)session.Lookup("o_uadd_assign");
        uint32_t* o_usub_assign = (uint32_t*)session.Lookup("o_usub_assign");
        uint32_t* o_umul_assign = (uint32_t*)session.Lookup("o_umul_assign");
        uint32_t* o_udiv_assign = (uint32_t*)session.Lookup("o_udiv_assign");
        uint32_t* o_urem_assign = (uint32_t*)session.Lookup("o_urem_assign");
        uint32_t* o_uband_assign = (uint32_t*)session.Lookup("o_uband_assign");
        uint32_t* o_ubor_assign = (uint32_t*)session.Lookup("o_ubor_assign");
        uint32_t* o_ubxor_assign = (uint32_t*)session.Lookup("o_ubxor_assign");
        uint32_t* o_ulshift_assign = (uint32_t*)session.Lookup("o_ulshift_assign");
        uint32_t* o_urshift_assign = (uint32_t*)session.Lookup("o_urshift_assign");

        // Verify all pointers are valid
        REQUIRE(o_add != nullptr);
        REQUIRE(o_sub != nullptr);
        REQUIRE(o_mul != nullptr);
        REQUIRE(o_div != nullptr);
        REQUIRE(o_fgreater != nullptr);
        REQUIRE(o_flesser != nullptr);
        REQUIRE(o_fgreater_eq != nullptr);
        REQUIRE(o_flesser_eq != nullptr);
        REQUIRE(o_fequal != nullptr);
        REQUIRE(o_fnot_equal != nullptr);
        REQUIRE(o_iadd != nullptr);
        REQUIRE(o_isub != nullptr);
        REQUIRE(o_imul != nullptr);
        REQUIRE(o_idiv != nullptr);
        REQUIRE(o_irem != nullptr);
        REQUIRE(o_band != nullptr);
        REQUIRE(o_bor != nullptr);
        REQUIRE(o_bxor != nullptr);
        REQUIRE(o_lshift != nullptr);
        REQUIRE(o_rshift != nullptr);
        REQUIRE(o_greater != nullptr);
        REQUIRE(o_lesser != nullptr);
        REQUIRE(o_greater_eq != nullptr);
        REQUIRE(o_lesser_eq != nullptr);
        REQUIRE(o_equal != nullptr);
        REQUIRE(o_not_equal != nullptr);
        REQUIRE(o_uadd != nullptr);
        REQUIRE(o_usub != nullptr);
        REQUIRE(o_umul != nullptr);
        REQUIRE(o_udiv != nullptr);
        REQUIRE(o_urem != nullptr);
        REQUIRE(o_uband != nullptr);
        REQUIRE(o_ubor != nullptr);
        REQUIRE(o_ubxor != nullptr);
        REQUIRE(o_ulshift != nullptr);
        REQUIRE(o_urshift != nullptr);
        REQUIRE(o_ugreater != nullptr);
        REQUIRE(o_uresser != nullptr);
        REQUIRE(o_ugreater_eq != nullptr);
        REQUIRE(o_uresser_eq != nullptr);
        REQUIRE(o_uequal != nullptr);
        REQUIRE(o_unot_equal != nullptr);
        REQUIRE(o_land != nullptr);
        REQUIRE(o_lor != nullptr);
        REQUIRE(o_assign != nullptr);
        REQUIRE(o_add_assign != nullptr);
        REQUIRE(o_sub_assign != nullptr);
        REQUIRE(o_mul_assign != nullptr);
        REQUIRE(o_div_assign != nullptr);
        REQUIRE(o_rem_assign != nullptr);
        REQUIRE(o_band_assign != nullptr);
        REQUIRE(o_bor_assign != nullptr);
        REQUIRE(o_bxor_assign != nullptr);
        REQUIRE(o_lshift_assign != nullptr);
        REQUIRE(o_rshift_assign != nullptr);
        REQUIRE(o_uadd_assign != nullptr);
        REQUIRE(o_usub_assign != nullptr);
        REQUIRE(o_umul_assign != nullptr);
        REQUIRE(o_udiv_assign != nullptr);
        REQUIRE(o_urem_assign != nullptr);
        REQUIRE(o_uband_assign != nullptr);
        REQUIRE(o_ubor_assign != nullptr);
        REQUIRE(o_ubxor_assign != nullptr);
        REQUIRE(o_ulshift_assign != nullptr);
        REQUIRE(o_urshift_assign != nullptr);

        // Execute the Main function
        void* main = session.Lookup("Main");
        REQUIRE(main != nullptr);
        ((void(*)())main)();

        // Verify float arithmetic operations
        REQUIRE(*o_add == (a + b));
        REQUIRE(*o_sub == (a - b));
        REQUIRE(*o_mul == (a * b));
        REQUIRE(*o_div == (a / b));

        // Verify float comparison operations
        REQUIRE(*o_fgreater == (a > b));
        REQUIRE(*o_flesser == (a < b));
        REQUIRE(*o_fgreater_eq == (a >= b));
        REQUIRE(*o_flesser_eq == (a <= b));
        REQUIRE(*o_fequal == (a == b));
        REQUIRE(*o_fnot_equal == (a != b));

        // Verify integer arithmetic
        REQUIRE(*o_iadd == (ia + ib));
        REQUIRE(*o_isub == (ia - ib));
        REQUIRE(*o_imul == (ia * ib));
        REQUIRE(*o_idiv == (ia / ib));
        REQUIRE(*o_irem == (ia % ib));

        // Verify integer bitwise operations
        REQUIRE(*o_band == (ia & ib));
        REQUIRE(*o_bor == (ia | ib));
        REQUIRE(*o_bxor == (ia ^ ib));
        REQUIRE(*o_lshift == (ia << 2));
        REQUIRE(*o_rshift == (ia >> 2));

        // Verify integer comparison operations
        REQUIRE(*o_greater == (ia > ib));
        REQUIRE(*o_lesser == (ia < ib));
        REQUIRE(*o_greater_eq == (ia >= ib));
        REQUIRE(*o_lesser_eq == (ia <= ib));
        REQUIRE(*o_equal == (ia == ib));
        REQUIRE(*o_not_equal == (ia != ib));

        // Verify unsigned arithmetic operations
        REQUIRE(*o_uadd == (ua + ub));
        REQUIRE(*o_usub == (ua - ub));
        REQUIRE(*o_umul == (ua * ub));
        REQUIRE(*o_udiv == (ua / ub));
        REQUIRE(*o_urem == (ua % ub));

        // Verify unsigned bitwise operations
        REQUIRE(*o_uband == (ua & ub));
        REQUIRE(*o_ubor == (ua | ub));
        REQUIRE(*o_ubxor == (ua ^ ub));
        REQUIRE(*o_ulshift == (ua << 2));
        REQUIRE(*o_urshift == (ua >> 2));

        // Verify unsigned comparison operations
        REQUIRE(*o_ugreater == (ua > ub));
        REQUIRE(*o_uresser == (ua < ub));
        REQUIRE(*o_ugreater_eq == (ua >= ub));
        REQUIRE(*o_uresser_eq == (ua <= ub));
        REQUIRE(*o_uequal == (ua == ub));
        REQUIRE(*o_unot_equal == (ua != ub));

        // Verify logical operations
        REQUIRE(*o_land == ((ia > 0) && (ib > 0)));
        REQUIRE(*o_lor == ((ia > 0) || (ib > 0)));

        // Verify float assignment operations
        REQUIRE(*o_assign == a);
        REQUIRE(*o_add_assign == (a + b));
        REQUIRE(*o_sub_assign == (a - b));
        REQUIRE(*o_mul_assign == (a * b));
        REQUIRE(*o_div_assign == (a / b));

        // Verify integer assignment operations
        REQUIRE(*o_rem_assign == (ia % ib));
        REQUIRE(*o_band_assign == (ia & ib));
        REQUIRE(*o_bor_assign == (ia | ib));
        REQUIRE(*o_bxor_assign == (ia ^ ib));
        REQUIRE(*o_lshift_assign == (ia << 2));
        REQUIRE(*o_rshift_assign == (ia >> 2));

        // Verify unsigned assignment operations
        REQUIRE(*o_uadd_assign == (ua + ub));
        REQUIRE(*o_usub_assign == (ua - ub));
        REQUIRE(*o_umul_assign == (ua * ub));
        REQUIRE(*o_udiv_assign == (ua / ub));
        REQUIRE(*o_urem_assign == (ua % ub));
        REQUIRE(*o_uband_assign == (ua & ub));
        REQUIRE(*o_ubor_assign == (ua | ub));
        REQUIRE(*o_ubxor_assign == (ua ^ ub));
        REQUIRE(*o_ulshift_assign == (ua << 2));
        REQUIRE(*o_urshift_assign == (ua >> 2));
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

TEST_CASE("Unary Expressions", "[Frontend]") {
    VCL::ExecutionSession session{};
    session.EnableGDBListener();
    REQUIRE(session.SubmitModule(MakeModule("VCL/unaryexpr.vcl")));

    SECTION("Value Check") {
        // Generate random input values for testing
        int32_t ia = GENERATE(Catch::Generators::take(1,
                Catch::Generators::random(-100, 100)));
        uint32_t ua = GENERATE(Catch::Generators::take(1,
                Catch::Generators::random(static_cast<uint32_t>(1), static_cast<uint32_t>(200))));
        bool ba = GENERATE(Catch::Generators::take(1,
                Catch::Generators::random(0, 1))) != 0;

        // Define input symbols
        REQUIRE(session.DefineSymbolPtr("ia", &ia));
        REQUIRE(session.DefineSymbolPtr("ua", &ua));
        REQUIRE(session.DefineSymbolPtr("ba", &ba));

        // Lookup output symbols for unary operations
        int32_t* o_prefix_inc = (int32_t*)session.Lookup("o_prefix_inc");
        int32_t* o_prefix_dec = (int32_t*)session.Lookup("o_prefix_dec");
        int32_t* o_plus = (int32_t*)session.Lookup("o_plus");
        int32_t* o_minus = (int32_t*)session.Lookup("o_minus");
        bool* o_logical_not = (bool*)session.Lookup("o_logical_not");
        uint32_t* o_bitwise_not = (uint32_t*)session.Lookup("o_bitwise_not");
        int32_t* o_postfix_inc = (int32_t*)session.Lookup("o_postfix_inc");
        int32_t* o_postfix_dec = (int32_t*)session.Lookup("o_postfix_dec");

        // Additional outputs to verify the actual value after increment/decrement
        int32_t* o_prefix_inc_result = (int32_t*)session.Lookup("o_prefix_inc_result");
        int32_t* o_prefix_dec_result = (int32_t*)session.Lookup("o_prefix_dec_result");
        int32_t* o_postfix_inc_result = (int32_t*)session.Lookup("o_postfix_inc_result");
        int32_t* o_postfix_dec_result = (int32_t*)session.Lookup("o_postfix_dec_result");

        // Verify all pointers are valid
        REQUIRE(o_prefix_inc != nullptr);
        REQUIRE(o_prefix_dec != nullptr);
        REQUIRE(o_plus != nullptr);
        REQUIRE(o_minus != nullptr);
        REQUIRE(o_logical_not != nullptr);
        REQUIRE(o_bitwise_not != nullptr);
        REQUIRE(o_postfix_inc != nullptr);
        REQUIRE(o_postfix_dec != nullptr);
        REQUIRE(o_prefix_inc_result != nullptr);
        REQUIRE(o_prefix_dec_result != nullptr);
        REQUIRE(o_postfix_inc_result != nullptr);
        REQUIRE(o_postfix_dec_result != nullptr);

        // Execute the Main function
        void* main = session.Lookup("Main");
        REQUIRE(main != nullptr);
        ((void(*)())main)();

        // Verify unary operations
        // Prefix increment: returns incremented value
        REQUIRE(*o_prefix_inc == (ia + 1));
        REQUIRE(*o_prefix_inc_result == (ia + 1));

        // Prefix decrement: returns decremented value
        REQUIRE(*o_prefix_dec == (ia - 1));
        REQUIRE(*o_prefix_dec_result == (ia - 1));

        // Unary plus: identity operation
        REQUIRE(*o_plus == ia);

        // Unary minus: negation
        REQUIRE(*o_minus == -ia);

        // Logical not
        REQUIRE(*o_logical_not == !ba);

        // Bitwise not
        REQUIRE(*o_bitwise_not == ~ua);

        // Postfix increment: returns original value, but variable is incremented
        REQUIRE(*o_postfix_inc == ia);  // Should return original value
        REQUIRE(*o_postfix_inc_result == (ia + 1));  // Variable should be incremented

        // Postfix decrement: returns original value, but variable is decremented
        REQUIRE(*o_postfix_dec == ia);  // Should return original value
        REQUIRE(*o_postfix_dec_result == (ia - 1));  // Variable should be decremented
    }
}

TEST_CASE("Vec Binary Expressions", "[Frontend]") {
    VCL::ExecutionSession session{};
    session.EnableGDBListener();

    ExpectedNoDiagnostic consumer{};
    VCL::CompilerContext cc{};
    cc.GetInvocation().GetDiagnosticOptions().SetDiagnosticConsumer(&consumer);
    cc.CreateDiagnosticEngine();
    cc.CreateIdentifierTable();
    cc.CreateSourceManager();
    cc.CreateTarget();
    cc.CreateLLVMContext();

    VCL::Source* source = cc.GetSourceManager().LoadFromDisk("VCL/vecbinaryexpr.vcl");
    REQUIRE(source != nullptr);

    VCL::EmitLLVMAction act{};

    std::shared_ptr<VCL::CompilerInstance> instance = cc.CreateInstance();
    instance->BeginSource(source);
    REQUIRE(instance->ExecuteAction(act));
    instance->EndSource();

    REQUIRE(session.SubmitModule(act.MoveModule()));

    VCL::StorageManager storage{ cc.GetTarget() };

    SECTION("Value Check") {
        // Vector width (8 elements for AVX2: 32 bytes / 4 = 8)
        uint32_t vectorWidth = cc.GetTarget().GetVectorWidthInElement();

        // Generate random input values for testing (aligned arrays)
        VCL::Float32VectorView vf32a = storage.AllocateFloat32Vector();
        VCL::Float32VectorView vf32b = storage.AllocateFloat32Vector();
        VCL::Float64VectorView vf64a = storage.AllocateFloat64Vector();
        VCL::Float64VectorView vf64b = storage.AllocateFloat64Vector();
        VCL::Int32VectorView vi32a = storage.AllocateInt32Vector();
        VCL::Int32VectorView vi32b = storage.AllocateInt32Vector();
        VCL::BoolVectorView vba = storage.AllocateBoolVector();
        VCL::BoolVectorView vbb = storage.AllocateBoolVector();

        // Initialize with random values
        for (size_t i = 0; i < vectorWidth; ++i) {
            vf32a[i] = GENERATE(Catch::Generators::take(1,
                    Catch::Generators::random(-100.0f, 100.0f)));
            vf32b[i] = GENERATE(Catch::Generators::take(1,
                    Catch::Generators::random(-100.0f, 100.0f)));
            vf64a[i] = GENERATE(Catch::Generators::take(1,
                    Catch::Generators::random(-100.0, 100.0)));
            vf64b[i] = GENERATE(Catch::Generators::take(1,
                    Catch::Generators::random(-100.0, 100.0)));
            vi32a[i] = GENERATE(Catch::Generators::take(1,
                    Catch::Generators::random(-1000, 1000)));
            vi32b[i] = GENERATE(Catch::Generators::take(1,
                    Catch::Generators::random(1, 1000))); // Avoid division by zero
            vba.Set(i, GENERATE(Catch::Generators::take(1,
                    Catch::Generators::random(0, 1))) != 0);
            vbb.Set(i, GENERATE(Catch::Generators::take(1,
                    Catch::Generators::random(0, 1))) != 0);
        }

        // Define input symbols
        REQUIRE(session.DefineSymbolPtr("vf32a", vf32a.GetPtr()));
        REQUIRE(session.DefineSymbolPtr("vf32b", vf32b.GetPtr()));
        REQUIRE(session.DefineSymbolPtr("vf64a", vf64a.GetPtr()));
        REQUIRE(session.DefineSymbolPtr("vf64b", vf64b.GetPtr()));
        REQUIRE(session.DefineSymbolPtr("vi32a", vi32a.GetPtr()));
        REQUIRE(session.DefineSymbolPtr("vi32b", vi32b.GetPtr()));
        REQUIRE(session.DefineSymbolPtr("vba", vba.GetPtr()));
        REQUIRE(session.DefineSymbolPtr("vbb", vbb.GetPtr()));

        // Lookup float32 vector output symbols
        VCL::Float32VectorView o_vf32_add = (float*)session.Lookup("o_vf32_add");
        VCL::Float32VectorView o_vf32_sub = (float*)session.Lookup("o_vf32_sub");
        VCL::Float32VectorView o_vf32_mul = (float*)session.Lookup("o_vf32_mul");
        VCL::Float32VectorView o_vf32_div = (float*)session.Lookup("o_vf32_div");
        VCL::BoolVectorView o_vf32_greater = (uint8_t*)session.Lookup("o_vf32_greater");
        VCL::BoolVectorView o_vf32_lesser = (uint8_t*)session.Lookup("o_vf32_lesser");
        VCL::BoolVectorView o_vf32_greater_eq = (uint8_t*)session.Lookup("o_vf32_greater_eq");
        VCL::BoolVectorView o_vf32_lesser_eq = (uint8_t*)session.Lookup("o_vf32_lesser_eq");
        VCL::BoolVectorView o_vf32_equal = (uint8_t*)session.Lookup("o_vf32_equal");
        VCL::BoolVectorView o_vf32_not_equal = (uint8_t*)session.Lookup("o_vf32_not_equal");
        VCL::Float32VectorView o_vf32_assign = (float*)session.Lookup("o_vf32_assign");
        VCL::Float32VectorView o_vf32_add_assign = (float*)session.Lookup("o_vf32_add_assign");
        VCL::Float32VectorView o_vf32_sub_assign = (float*)session.Lookup("o_vf32_sub_assign");
        VCL::Float32VectorView o_vf32_mul_assign = (float*)session.Lookup("o_vf32_mul_assign");
        VCL::Float32VectorView o_vf32_div_assign = (float*)session.Lookup("o_vf32_div_assign");

        // Lookup float64 vector output symbols
        VCL::Float64VectorView o_vf64_add = (double*)session.Lookup("o_vf64_add");
        VCL::Float64VectorView o_vf64_sub = (double*)session.Lookup("o_vf64_sub");
        VCL::Float64VectorView o_vf64_mul = (double*)session.Lookup("o_vf64_mul");
        VCL::Float64VectorView o_vf64_div = (double*)session.Lookup("o_vf64_div");
        VCL::BoolVectorView o_vf64_greater = (uint8_t*)session.Lookup("o_vf64_greater");
        VCL::BoolVectorView o_vf64_lesser = (uint8_t*)session.Lookup("o_vf64_lesser");
        VCL::BoolVectorView o_vf64_greater_eq = (uint8_t*)session.Lookup("o_vf64_greater_eq");
        VCL::BoolVectorView o_vf64_lesser_eq = (uint8_t*)session.Lookup("o_vf64_lesser_eq");
        VCL::BoolVectorView o_vf64_equal = (uint8_t*)session.Lookup("o_vf64_equal");
        VCL::BoolVectorView o_vf64_not_equal = (uint8_t*)session.Lookup("o_vf64_not_equal");
        VCL::Float64VectorView o_vf64_assign = (double*)session.Lookup("o_vf64_assign");
        VCL::Float64VectorView o_vf64_add_assign = (double*)session.Lookup("o_vf64_add_assign");
        VCL::Float64VectorView o_vf64_sub_assign = (double*)session.Lookup("o_vf64_sub_assign");
        VCL::Float64VectorView o_vf64_mul_assign = (double*)session.Lookup("o_vf64_mul_assign");
        VCL::Float64VectorView o_vf64_div_assign = (double*)session.Lookup("o_vf64_div_assign");

        // Lookup int32 vector output symbols
        VCL::Int32VectorView o_vi32_add = (int32_t*)session.Lookup("o_vi32_add");
        VCL::Int32VectorView o_vi32_sub = (int32_t*)session.Lookup("o_vi32_sub");
        VCL::Int32VectorView o_vi32_mul = (int32_t*)session.Lookup("o_vi32_mul");
        VCL::Int32VectorView o_vi32_div = (int32_t*)session.Lookup("o_vi32_div");
        VCL::Int32VectorView o_vi32_rem = (int32_t*)session.Lookup("o_vi32_rem");
        VCL::Int32VectorView o_vi32_band = (int32_t*)session.Lookup("o_vi32_band");
        VCL::Int32VectorView o_vi32_bor = (int32_t*)session.Lookup("o_vi32_bor");
        VCL::Int32VectorView o_vi32_bxor = (int32_t*)session.Lookup("o_vi32_bxor");
        VCL::Int32VectorView o_vi32_lshift = (int32_t*)session.Lookup("o_vi32_lshift");
        VCL::Int32VectorView o_vi32_rshift = (int32_t*)session.Lookup("o_vi32_rshift");
        VCL::BoolVectorView o_vi32_greater = (uint8_t*)session.Lookup("o_vi32_greater");
        VCL::BoolVectorView o_vi32_lesser = (uint8_t*)session.Lookup("o_vi32_lesser");
        VCL::BoolVectorView o_vi32_greater_eq = (uint8_t*)session.Lookup("o_vi32_greater_eq");
        VCL::BoolVectorView o_vi32_lesser_eq = (uint8_t*)session.Lookup("o_vi32_lesser_eq");
        VCL::BoolVectorView o_vi32_equal = (uint8_t*)session.Lookup("o_vi32_equal");
        VCL::BoolVectorView o_vi32_not_equal = (uint8_t*)session.Lookup("o_vi32_not_equal");
        VCL::Int32VectorView o_vi32_assign = (int32_t*)session.Lookup("o_vi32_assign");
        VCL::Int32VectorView o_vi32_add_assign = (int32_t*)session.Lookup("o_vi32_add_assign");
        VCL::Int32VectorView o_vi32_sub_assign = (int32_t*)session.Lookup("o_vi32_sub_assign");
        VCL::Int32VectorView o_vi32_mul_assign = (int32_t*)session.Lookup("o_vi32_mul_assign");
        VCL::Int32VectorView o_vi32_div_assign = (int32_t*)session.Lookup("o_vi32_div_assign");
        VCL::Int32VectorView o_vi32_rem_assign = (int32_t*)session.Lookup("o_vi32_rem_assign");
        VCL::Int32VectorView o_vi32_band_assign = (int32_t*)session.Lookup("o_vi32_band_assign");
        VCL::Int32VectorView o_vi32_bor_assign = (int32_t*)session.Lookup("o_vi32_bor_assign");
        VCL::Int32VectorView o_vi32_bxor_assign = (int32_t*)session.Lookup("o_vi32_bxor_assign");
        VCL::Int32VectorView o_vi32_lshift_assign = (int32_t*)session.Lookup("o_vi32_lshift_assign");
        VCL::Int32VectorView o_vi32_rshift_assign = (int32_t*)session.Lookup("o_vi32_rshift_assign");

        // Lookup bool vector output symbols
        VCL::BoolVectorView o_vb_land = (uint8_t*)session.Lookup("o_vb_land");
        VCL::BoolVectorView o_vb_lor = (uint8_t*)session.Lookup("o_vb_lor");
        VCL::BoolVectorView o_vb_equal = (uint8_t*)session.Lookup("o_vb_equal");
        VCL::BoolVectorView o_vb_not_equal = (uint8_t*)session.Lookup("o_vb_not_equal");

        // Verify all pointers are valid
        REQUIRE(o_vf32_add.IsValid());
        REQUIRE(o_vf32_sub.IsValid());
        REQUIRE(o_vf32_mul.IsValid());
        REQUIRE(o_vf32_div.IsValid());
        REQUIRE(o_vf32_greater.IsValid());
        REQUIRE(o_vf32_lesser.IsValid());
        REQUIRE(o_vf32_greater_eq.IsValid());
        REQUIRE(o_vf32_lesser_eq.IsValid());
        REQUIRE(o_vf32_equal.IsValid());
        REQUIRE(o_vf32_not_equal.IsValid());
        REQUIRE(o_vf32_assign.IsValid());
        REQUIRE(o_vf32_add_assign.IsValid());
        REQUIRE(o_vf32_sub_assign.IsValid());
        REQUIRE(o_vf32_mul_assign.IsValid());
        REQUIRE(o_vf32_div_assign.IsValid());

        REQUIRE(o_vf64_add.IsValid());
        REQUIRE(o_vf64_sub.IsValid());
        REQUIRE(o_vf64_mul.IsValid());
        REQUIRE(o_vf64_div.IsValid());
        REQUIRE(o_vf64_greater.IsValid());
        REQUIRE(o_vf64_lesser.IsValid());
        REQUIRE(o_vf64_greater_eq.IsValid());
        REQUIRE(o_vf64_lesser_eq.IsValid());
        REQUIRE(o_vf64_equal.IsValid());
        REQUIRE(o_vf64_not_equal.IsValid());
        REQUIRE(o_vf64_assign.IsValid());
        REQUIRE(o_vf64_add_assign.IsValid());
        REQUIRE(o_vf64_sub_assign.IsValid());
        REQUIRE(o_vf64_mul_assign.IsValid());
        REQUIRE(o_vf64_div_assign.IsValid());

        REQUIRE(o_vi32_add.IsValid());
        REQUIRE(o_vi32_sub.IsValid());
        REQUIRE(o_vi32_mul.IsValid());
        REQUIRE(o_vi32_div.IsValid());
        REQUIRE(o_vi32_rem.IsValid());
        REQUIRE(o_vi32_band.IsValid());
        REQUIRE(o_vi32_bor.IsValid());
        REQUIRE(o_vi32_bxor.IsValid());
        REQUIRE(o_vi32_lshift.IsValid());
        REQUIRE(o_vi32_rshift.IsValid());
        REQUIRE(o_vi32_greater.IsValid());
        REQUIRE(o_vi32_lesser.IsValid());
        REQUIRE(o_vi32_greater_eq.IsValid());
        REQUIRE(o_vi32_lesser_eq.IsValid());
        REQUIRE(o_vi32_equal.IsValid());
        REQUIRE(o_vi32_not_equal.IsValid());
        REQUIRE(o_vi32_assign.IsValid());
        REQUIRE(o_vi32_add_assign.IsValid());
        REQUIRE(o_vi32_sub_assign.IsValid());
        REQUIRE(o_vi32_mul_assign.IsValid());
        REQUIRE(o_vi32_div_assign.IsValid());
        REQUIRE(o_vi32_rem_assign.IsValid());
        REQUIRE(o_vi32_band_assign.IsValid());
        REQUIRE(o_vi32_bor_assign.IsValid());
        REQUIRE(o_vi32_bxor_assign.IsValid());
        REQUIRE(o_vi32_lshift_assign.IsValid());
        REQUIRE(o_vi32_rshift_assign.IsValid());

        REQUIRE(o_vb_land.IsValid());
        REQUIRE(o_vb_lor.IsValid());
        REQUIRE(o_vb_equal.IsValid());
        REQUIRE(o_vb_not_equal.IsValid());

        // Execute the Main function
        void* main = session.Lookup("Main");
        REQUIRE(main != nullptr);
        ((void(*)())main)();

        // Verify float32 vector operations (element-wise)
        for (size_t i = 0; i < vectorWidth; ++i) {
            INFO(std::format("vf32a[{}] == {} && vf32b[{}] == {}", i, vf32a[i], i, vf32b[i]));
            REQUIRE(o_vf32_add[i] == (vf32a[i] + vf32b[i]));
            REQUIRE(o_vf32_sub[i] == (vf32a[i] - vf32b[i]));
            REQUIRE(o_vf32_mul[i] == (vf32a[i] * vf32b[i]));
            REQUIRE(o_vf32_div[i] == (vf32a[i] / vf32b[i]));
            REQUIRE(o_vf32_greater.Get(i) == (vf32a[i] > vf32b[i]));
            REQUIRE(o_vf32_lesser.Get(i) == (vf32a[i] < vf32b[i]));
            REQUIRE(o_vf32_greater_eq.Get(i) == (vf32a[i] >= vf32b[i]));
            REQUIRE(o_vf32_lesser_eq.Get(i) == (vf32a[i] <= vf32b[i]));
            REQUIRE(o_vf32_equal.Get(i) == (vf32a[i] == vf32b[i]));
            REQUIRE(o_vf32_not_equal.Get(i) == (vf32a[i] != vf32b[i]));
            REQUIRE(o_vf32_assign[i] == vf32a[i]);
            REQUIRE(o_vf32_add_assign[i] == (vf32a[i] + vf32b[i]));
            REQUIRE(o_vf32_sub_assign[i] == (vf32a[i] - vf32b[i]));
            REQUIRE(o_vf32_mul_assign[i] == (vf32a[i] * vf32b[i]));
            REQUIRE(o_vf32_div_assign[i] == (vf32a[i] / vf32b[i]));
        }

        // Verify float64 vector operations (element-wise)
        for (size_t i = 0; i < vectorWidth; ++i) {
            INFO(std::format("vf64a[{}] == {} && vf64b[{}] == {}", i, vf64a[i], i, vf64b[i]));
            REQUIRE(o_vf64_add[i] == (vf64a[i] + vf64b[i]));
            REQUIRE(o_vf64_sub[i] == (vf64a[i] - vf64b[i]));
            REQUIRE(o_vf64_mul[i] == (vf64a[i] * vf64b[i]));
            REQUIRE(o_vf64_div[i] == (vf64a[i] / vf64b[i]));
            REQUIRE(o_vf64_greater.Get(i) == (vf64a[i] > vf64b[i]));
            REQUIRE(o_vf64_lesser.Get(i) == (vf64a[i] < vf64b[i]));
            REQUIRE(o_vf64_greater_eq.Get(i) == (vf64a[i] >= vf64b[i]));
            REQUIRE(o_vf64_lesser_eq.Get(i) == (vf64a[i] <= vf64b[i]));
            REQUIRE(o_vf64_equal.Get(i) == (vf64a[i] == vf64b[i]));
            REQUIRE(o_vf64_not_equal.Get(i) == (vf64a[i] != vf64b[i]));
            REQUIRE(o_vf64_assign[i] == vf64a[i]);
            REQUIRE(o_vf64_add_assign[i] == (vf64a[i] + vf64b[i]));
            REQUIRE(o_vf64_sub_assign[i] == (vf64a[i] - vf64b[i]));
            REQUIRE(o_vf64_mul_assign[i] == (vf64a[i] * vf64b[i]));
            REQUIRE(o_vf64_div_assign[i] == (vf64a[i] / vf64b[i]));
        }

        // Verify int32 vector operations (element-wise)
        for (size_t i = 0; i < vectorWidth; ++i) {
            INFO(std::format("vi32a[{}] == {} && vi32b[{}] == {}", i, vi32a[i], i, vi32b[i]));
            REQUIRE(o_vi32_add[i] == (vi32a[i] + vi32b[i]));
            REQUIRE(o_vi32_sub[i] == (vi32a[i] - vi32b[i]));
            REQUIRE(o_vi32_mul[i] == (vi32a[i] * vi32b[i]));
            REQUIRE(o_vi32_div[i] == (vi32a[i] / vi32b[i]));
            REQUIRE(o_vi32_rem[i] == (vi32a[i] % vi32b[i]));
            REQUIRE(o_vi32_band[i] == (vi32a[i] & vi32b[i]));
            REQUIRE(o_vi32_bor[i] == (vi32a[i] | vi32b[i]));
            REQUIRE(o_vi32_bxor[i] == (vi32a[i] ^ vi32b[i]));
            REQUIRE(o_vi32_lshift[i] == (vi32a[i] << 2));
            REQUIRE(o_vi32_rshift[i] == (vi32a[i] >> 2));
            REQUIRE(o_vi32_greater.Get(i) == (vi32a[i] > vi32b[i]));
            REQUIRE(o_vi32_lesser.Get(i) == (vi32a[i] < vi32b[i]));
            REQUIRE(o_vi32_greater_eq.Get(i) == (vi32a[i] >= vi32b[i]));
            REQUIRE(o_vi32_lesser_eq.Get(i) == (vi32a[i] <= vi32b[i]));
            REQUIRE(o_vi32_equal.Get(i) == (vi32a[i] == vi32b[i]));
            REQUIRE(o_vi32_not_equal.Get(i) == (vi32a[i] != vi32b[i]));
            REQUIRE(o_vi32_assign[i] == vi32a[i]);
            REQUIRE(o_vi32_add_assign[i] == (vi32a[i] + vi32b[i]));
            REQUIRE(o_vi32_sub_assign[i] == (vi32a[i] - vi32b[i]));
            REQUIRE(o_vi32_mul_assign[i] == (vi32a[i] * vi32b[i]));
            REQUIRE(o_vi32_div_assign[i] == (vi32a[i] / vi32b[i]));
            REQUIRE(o_vi32_rem_assign[i] == (vi32a[i] % vi32b[i]));
            REQUIRE(o_vi32_band_assign[i] == (vi32a[i] & vi32b[i]));
            REQUIRE(o_vi32_bor_assign[i] == (vi32a[i] | vi32b[i]));
            REQUIRE(o_vi32_bxor_assign[i] == (vi32a[i] ^ vi32b[i]));
            REQUIRE(o_vi32_lshift_assign[i] == (vi32a[i] << 2));
            REQUIRE(o_vi32_rshift_assign[i] == (vi32a[i] >> 2));
        }

        // Verify bool vector operations (element-wise)
        for (size_t i = 0; i < vectorWidth; ++i) {
            INFO(std::format("vba[{}] == {} && vbb[{}] == {}", i, vba.Get(i), i, vbb.Get(i)));
            REQUIRE(o_vb_land.Get(i) == (vba.Get(i) && vbb.Get(i)));
            REQUIRE(o_vb_lor.Get(i) == (vba.Get(i) || vbb.Get(i)));
            REQUIRE(o_vb_equal.Get(i) == (vba.Get(i) == vbb.Get(i)));
            REQUIRE(o_vb_not_equal.Get(i) == (vba.Get(i) != vbb.Get(i)));
        }
    }
}

struct SpanTest {
	int32_t* ptr;
	uint64_t size;
};

TEST_CASE("Subscript Expressions", "[Frontend]") {
    VCL::ExecutionSession session{};
    session.EnableGDBListener();
    REQUIRE(session.SubmitModule(MakeModule("VCL/subscriptexpr.vcl")));

    SECTION("Value Check") {
        constexpr uint32_t arraySize = 32;

		int32_t* array = new int32_t[arraySize]{};
		SpanTest span{ new int32_t[arraySize]{}, arraySize };
		
		uint32_t index = GENERATE(Catch::Generators::take(1,
                Catch::Generators::random(0u, arraySize - 1)));

		for (uint32_t i = 0; i < arraySize; ++i) {
			array[i] = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(-1000, 1000)));

			span.ptr[i] = GENERATE(Catch::Generators::take(1, 
                Catch::Generators::random(-1000, 1000)));
		}

        REQUIRE(session.DefineSymbolPtr("array", array));
        REQUIRE(session.DefineSymbolPtr("span", &span));
        REQUIRE(session.DefineSymbolPtr("index", &index));

        int32_t* arrayOut = (int32_t*)session.Lookup("arrayOut");
        int32_t* spanOut = (int32_t*)session.Lookup("spanOut");

        REQUIRE(arrayOut != nullptr);
        REQUIRE(spanOut != nullptr);

        void* main = session.Lookup("Main");
        REQUIRE(main != nullptr);
        ((void(*)())main)();

        REQUIRE(*arrayOut == array[index]);
        REQUIRE(*spanOut == span.ptr[index]);
    }
}