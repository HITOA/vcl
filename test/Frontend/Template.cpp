#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>

#include <VCL/Core/SourceManager.hpp>
#include <VCL/Core/Source.hpp>
#include <VCL/Frontend/CompilerContext.hpp>
#include <VCL/Frontend/CompilerInstance.hpp>
#include <VCL/Frontend/FrontendActions.hpp>
#include <VCL/Frontend/ExecutionSession.hpp>
#include <VCL/Frontend/Storage.hpp>

#include "../Common/ExpectedDiagnostic.hpp"
#include "../Common/MakeModule.hpp"


// =============================================================================
// Template Function Tests
// =============================================================================

TEST_CASE("Template Functions", "[Template][Function]") {
    VCL::ExecutionSession session{};
    session.EnableGDBListener();
    REQUIRE(session.SubmitModule(MakeModule("VCL/templatefunc.vcl")));

    SECTION("Value Check") {
        // Generate random input values
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
        float* o_add = (float*)session.Lookup("o_add");
        int32_t* o_iadd = (int32_t*)session.Lookup("o_iadd");
        float* o_abs = (float*)session.Lookup("o_abs");
        float* o_max = (float*)session.Lookup("o_max");
        float* o_lerp = (float*)session.Lookup("o_lerp");
        int32_t* o_nested = (int32_t*)session.Lookup("o_nested");
        float* o_explicit = (float*)session.Lookup("o_explicit");

        // Verify all pointers are valid
        REQUIRE(o_add != nullptr);
        REQUIRE(o_iadd != nullptr);
        REQUIRE(o_abs != nullptr);
        REQUIRE(o_max != nullptr);
        REQUIRE(o_lerp != nullptr);
        REQUIRE(o_nested != nullptr);
        REQUIRE(o_explicit != nullptr);

        // Execute the Main function
        void* main = session.Lookup("Main");
        REQUIRE(main != nullptr);
        ((void(*)())main)();

        // Verify template function results
        // Template argument deduction
        REQUIRE(*o_add == (a + b));
        REQUIRE(*o_iadd == (ia + ib));

        // Abs function (branchless)
        float zero_f = 0.0f;
        REQUIRE(*o_abs == (a * ((a > zero_f) - (a < zero_f))));

        // Max function
        float expected_max = (a > b) * a + (a <= b) * b;
        REQUIRE(*o_max == expected_max);

        // Lerp function
        REQUIRE(*o_lerp == (a + (b - a) * 0.5f));

        // Nested template function calls (AbsMax)
        int32_t zero_i = 0;
        int32_t absX = ia * ((ia > zero_i) - (ia < zero_i));
        int32_t absY = ib * ((ib > zero_i) - (ib < zero_i));
        int32_t expected_nested = (absX > absY) * absX + (absX <= absY) * absY;
        REQUIRE(*o_nested == expected_nested);

        // Explicit template argument
        REQUIRE(*o_explicit == a);
    }
}


// =============================================================================
// Template Struct Tests
// =============================================================================

TEST_CASE("Template Structs", "[Template][Struct]") {
    VCL::ExecutionSession session{};
    session.EnableGDBListener();

    ExpectedNoDiagnostic consumer{};
    VCL::CompilerContext cc{};
    cc.GetInvocation().GetDiagnosticOptions().SetDiagnosticConsumer(&consumer);
    cc.CreateDiagnosticEngine();
    cc.CreateIdentifierTable();
    cc.CreateAttributeTable();
    cc.CreateDirectiveRegistry();
    cc.CreateSourceManager();
    cc.CreateTypeCache();
    cc.CreateTarget();
    cc.CreateLLVMContext();

    VCL::Source* source = cc.GetSourceManager().LoadFromDisk("VCL/templatestruct.vcl");
    REQUIRE(source != nullptr);

    VCL::EmitLLVMAction act{};

    std::shared_ptr<VCL::CompilerInstance> instance = cc.CreateInstance();
    instance->BeginSource(source);
    REQUIRE(instance->ExecuteAction(act));
    instance->EndSource();

    REQUIRE(session.SubmitModule(act.MoveModule()));

    VCL::StorageManager storage{ cc.GetTarget() };

    SECTION("Value Check") {
        // Generate random input values
        float a = GENERATE(Catch::Generators::take(1,
                Catch::Generators::random(-50.0f, 50.0f)));
        float b = GENERATE(Catch::Generators::take(1,
                Catch::Generators::random(-50.0f, 50.0f)));
        int32_t ia = GENERATE(Catch::Generators::take(1,
                Catch::Generators::random(-100, 100)));

        // Define input symbols
        REQUIRE(session.DefineSymbolPtr("a", &a));
        REQUIRE(session.DefineSymbolPtr("b", &b));
        REQUIRE(session.DefineSymbolPtr("ia", &ia));

        // Lookup output symbols
        float* o_pair_first = (float*)session.Lookup("o_pair_first");
        float* o_pair_second = (float*)session.Lookup("o_pair_second");
        float* o_buffer_val = (float*)session.Lookup("o_buffer_val");
        int32_t* o_wrapper_val = (int32_t*)session.Lookup("o_wrapper_val");
        VCL::Float32VectorView o_nested_val = (float*)session.Lookup("o_nested_val");

        // Verify all pointers are valid
        REQUIRE(o_pair_first != nullptr);
        REQUIRE(o_pair_second != nullptr);
        REQUIRE(o_buffer_val != nullptr);
        REQUIRE(o_wrapper_val != nullptr);
        REQUIRE(o_nested_val.IsValid());

        // Execute the Main function
        void* main = session.Lookup("Main");
        REQUIRE(main != nullptr);
        ((void(*)())main)();

        // Verify template struct results
        REQUIRE(*o_pair_first == a);
        REQUIRE(*o_pair_second == b);
        REQUIRE(*o_buffer_val == a);
        REQUIRE(*o_wrapper_val == ia);
        REQUIRE(o_nested_val.Get(0) == 1.0f);
    }
}


// =============================================================================
// Template Argument Deduction Tests
// =============================================================================

TEST_CASE("Template Argument Deduction", "[Template][Deduction]") {
    VCL::ExecutionSession session{};
    session.EnableGDBListener();
    REQUIRE(session.SubmitModule(MakeModule("VCL/templatededuction.vcl")));

    SECTION("Value Check") {
        // Generate random input values
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
        float* o_deduced1 = (float*)session.Lookup("o_deduced1");
        int32_t* o_deduced2 = (int32_t*)session.Lookup("o_deduced2");
        float* o_deduced3 = (float*)session.Lookup("o_deduced3");
        float* o_multi1 = (float*)session.Lookup("o_multi1");
        int32_t* o_multi2 = (int32_t*)session.Lookup("o_multi2");

        // Verify all pointers are valid
        REQUIRE(o_deduced1 != nullptr);
        REQUIRE(o_deduced2 != nullptr);
        REQUIRE(o_deduced3 != nullptr);
        REQUIRE(o_multi1 != nullptr);
        REQUIRE(o_multi2 != nullptr);

        // Execute the Main function
        void* main = session.Lookup("Main");
        REQUIRE(main != nullptr);
        ((void(*)())main)();

        // Verify template argument deduction
        REQUIRE(*o_deduced1 == a);
        REQUIRE(*o_deduced2 == ia);
        REQUIRE(*o_deduced3 == ((a + b) + (a + b)));
        REQUIRE(*o_multi1 == (a + b));
        REQUIRE(*o_multi2 == (ia + ib));
    }
}


// =============================================================================
// Dependent Expression Tests
// =============================================================================

TEST_CASE("Template Dependent Expressions", "[Template][Dependent]") {
    VCL::ExecutionSession session{};
    session.EnableGDBListener();
    REQUIRE(session.SubmitModule(MakeModule("VCL/templatedependent.vcl")));

    SECTION("Value Check") {
        // Generate random input values
        float a = GENERATE(Catch::Generators::take(1,
                Catch::Generators::random(-50.0f, 50.0f)));
        float b = GENERATE(Catch::Generators::take(1,
                Catch::Generators::random(-50.0f, 50.0f)));
        int32_t ia = GENERATE(Catch::Generators::take(1,
                Catch::Generators::random(-100, 100)));

        // Define input symbols
        REQUIRE(session.DefineSymbolPtr("a", &a));
        REQUIRE(session.DefineSymbolPtr("b", &b));
        REQUIRE(session.DefineSymbolPtr("ia", &ia));

        // Lookup output symbols
        float* o_field_access = (float*)session.Lookup("o_field_access");
        float* o_binary_op = (float*)session.Lookup("o_binary_op");
        float* o_unary_op = (float*)session.Lookup("o_unary_op");
        float* o_call_chain = (float*)session.Lookup("o_call_chain");
        float* o_subscript = (float*)session.Lookup("o_subscript");
        float* o_cast = (float*)session.Lookup("o_cast");

        // Verify all pointers are valid
        REQUIRE(o_field_access != nullptr);
        REQUIRE(o_binary_op != nullptr);
        REQUIRE(o_unary_op != nullptr);
        REQUIRE(o_call_chain != nullptr);
        REQUIRE(o_subscript != nullptr);
        REQUIRE(o_cast != nullptr);

        // Execute the Main function
        void* main = session.Lookup("Main");
        REQUIRE(main != nullptr);
        ((void(*)())main)();

        // Verify dependent expression results
        REQUIRE(*o_field_access == a);

        float sum = a + b;
        float diff = a - b;
        float prod = a * b;
        float expected_binary = sum - prod;
        REQUIRE(*o_binary_op == expected_binary);

        REQUIRE(*o_unary_op == -a);

        // ChainedCall: Helper(Helper(value)) where Helper(val) = val * 2
        float expected_chain = ((a * 2.0f) * 2.0f);
        REQUIRE(*o_call_chain == expected_chain);

        REQUIRE(*o_subscript == a);
        REQUIRE(*o_cast == static_cast<float>(ia));
    }
}


// =============================================================================
// Template Error Tests
// =============================================================================

// Helper function to check for specific errors
template<VCL::Diagnostic::DiagnosticMsg Message>
void CheckForError(const char* src) {
    ExpectedDiagnostic<Message> consumer{};
    VCL::CompilerContext cc{};
    cc.GetInvocation().GetDiagnosticOptions().SetDiagnosticConsumer(&consumer);
    cc.CreateDiagnosticEngine();
    cc.CreateIdentifierTable();
    cc.CreateAttributeTable();
    cc.CreateDirectiveRegistry();
    cc.CreateTypeCache();
    cc.CreateSourceManager();

    VCL::Source* source = cc.GetSourceManager().LoadFromMemory(src);
    REQUIRE(source != nullptr);

    VCL::ParseSyntaxOnlyAction act{};

    std::shared_ptr<VCL::CompilerInstance> instance = cc.CreateInstance();
    instance->BeginSource(source);
    instance->ExecuteAction(act);
    instance->EndSource();

    consumer.Require();
}

TEST_CASE("Template Error - Too many arguments", "[Template][Error]") {
    CheckForError<VCL::Diagnostic::TooManyTemplateArgument>(R"(
        template<typename T>
        struct Wrapper {
            T value;
        }

        Wrapper<float32, int32> w;
    )");
}

TEST_CASE("Template Error - Not enough arguments", "[Template][Error]") {
    CheckForError<VCL::Diagnostic::NotEnoughTemplateArgument>(R"(
        template<typename T, typename U>
        struct Pair {
            T first;
            U second;
        }

        Pair<float32> p;
    )");
}

TEST_CASE("Template Error - Missing arguments", "[Template][Error]") {
    CheckForError<VCL::Diagnostic::MissingTemplateArgument>(R"(
        template<typename T>
        struct Wrapper {
            T value;
        }

        Wrapper w;
    )");
}

TEST_CASE("Template Error - Wrong argument type (type expected)", "[Template][Error]") {
    CheckForError<VCL::Diagnostic::TemplateArgumentWrongType>(R"(
        template<typename T>
        struct Wrapper {
            T value;
        }

        Wrapper<42> w;
    )");
}

TEST_CASE("Template Error - Wrong argument type (integral expected)", "[Template][Error]") {
    CheckForError<VCL::Diagnostic::TemplateArgumentWrongType>(R"(
        template<typename T, uint64 Size>
        struct Buffer {
            Array<T, Size> data;
        }

        Buffer<float32, float32> buf;
    )");
}

TEST_CASE("Template Error - Non-template with template args", "[Template][Error]") {
    CheckForError<VCL::Diagnostic::DoesNotTakeTemplateArgList>(R"(
        struct SimpleStruct {
            float32 value;
        }

        SimpleStruct<float32> s;
    )");
}

TEST_CASE("Template Error - Template parameter redeclaration", "[Template][Error]") {
    CheckForError<VCL::Diagnostic::TemplateRedeclared>(R"(
        template<typename T, typename T>
        struct Bad {
            T value;
        }
    )");
}

TEST_CASE("Template Error - Non-constant template argument", "[Template][Error]") {
    CheckForError<VCL::Diagnostic::ExprDoesNotEvaluate>(R"(
        float32 GetSize();

        template<typename T, uint64 Size>
        struct Buffer {
            Array<T, Size> data;
        }

        void Main() {
            Buffer<float32, GetSize()> buf;
        }
    )");
}
