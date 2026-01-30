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


TEST_CASE("If Statements", "[Frontend][ControlFlow]") {
    VCL::ExecutionSession session{};
    session.EnableGDBListener();
    REQUIRE(session.SubmitModule(MakeModule("VCL/controlflow.vcl")));

    SECTION("Value Check") {
        // Generate random condition value for testing if statements
        int32_t condition_value = GENERATE(
                -10, -5, -1,    // Negative values
                0,              // Zero
                1, 3, 5, 9,     // Small positive (1-9)
                10, 15, 20      // Large positive (>=10)
        );
        int32_t loop_count = 10;  // Not used for if tests, but required by VCL

        // Define input symbols
        REQUIRE(session.DefineSymbolPtr("condition_value", &condition_value));
        REQUIRE(session.DefineSymbolPtr("loop_count", &loop_count));

        // Lookup output symbols
        int32_t* o_if_simple = (int32_t*)session.Lookup("o_if_simple");
        int32_t* o_if_else = (int32_t*)session.Lookup("o_if_else");
        int32_t* o_if_elseif_else = (int32_t*)session.Lookup("o_if_elseif_else");
        int32_t* o_if_nested = (int32_t*)session.Lookup("o_if_nested");

        // Verify all pointers are valid
        REQUIRE(o_if_simple != nullptr);
        REQUIRE(o_if_else != nullptr);
        REQUIRE(o_if_elseif_else != nullptr);
        REQUIRE(o_if_nested != nullptr);

        // Execute the Main function
        void* main = session.Lookup("Main");
        REQUIRE(main != nullptr);
        ((void(*)())main)();

        // Verify simple if
        int32_t expected_if_simple = (condition_value > 0) ? 1 : 0;
        INFO(std::format("condition_value={}, expected={}, actual={}",
                condition_value, expected_if_simple, *o_if_simple));
        REQUIRE(*o_if_simple == expected_if_simple);

        // Verify if-else
        int32_t expected_if_else = (condition_value > 10) ? 1 : 2;
        INFO(std::format("condition_value={}, expected={}, actual={}",
                condition_value, expected_if_else, *o_if_else));
        REQUIRE(*o_if_else == expected_if_else);

        // Verify if-else if-else chain
        int32_t expected_if_elseif_else;
        if (condition_value < 0) {
            expected_if_elseif_else = 1;
        } else if (condition_value == 0) {
            expected_if_elseif_else = 2;
        } else if (condition_value < 10) {
            expected_if_elseif_else = 3;
        } else {
            expected_if_elseif_else = 4;
        }
        INFO(std::format("condition_value={}, expected={}, actual={}",
                condition_value, expected_if_elseif_else, *o_if_elseif_else));
        REQUIRE(*o_if_elseif_else == expected_if_elseif_else);

        // Verify nested if
        int32_t expected_if_nested;
        if (condition_value >= 0) {
            if (condition_value < 5) {
                expected_if_nested = 1;
            } else {
                if (condition_value < 10) {
                    expected_if_nested = 2;
                } else {
                    expected_if_nested = 3;
                }
            }
        } else {
            expected_if_nested = 4;
        }
        INFO(std::format("condition_value={}, expected={}, actual={}",
                condition_value, expected_if_nested, *o_if_nested));
        REQUIRE(*o_if_nested == expected_if_nested);
    }
}


TEST_CASE("While Loops", "[Frontend][ControlFlow]") {
    VCL::ExecutionSession session{};
    session.EnableGDBListener();
    REQUIRE(session.SubmitModule(MakeModule("VCL/controlflow.vcl")));

    SECTION("Value Check") {
        // Generate random loop count
        int32_t loop_count = GENERATE(Catch::Generators::take(1,
                Catch::Generators::random(5, 20)));
        int32_t condition_value = 5;  // Not used for while tests, but required by VCL

        // Define input symbols
        REQUIRE(session.DefineSymbolPtr("condition_value", &condition_value));
        REQUIRE(session.DefineSymbolPtr("loop_count", &loop_count));

        // Lookup output symbols
        int32_t* o_while_basic = (int32_t*)session.Lookup("o_while_basic");
        int32_t* o_while_break = (int32_t*)session.Lookup("o_while_break");
        int32_t* o_while_continue = (int32_t*)session.Lookup("o_while_continue");

        // Verify all pointers are valid
        REQUIRE(o_while_basic != nullptr);
        REQUIRE(o_while_break != nullptr);
        REQUIRE(o_while_continue != nullptr);

        // Execute the Main function
        void* main = session.Lookup("Main");
        REQUIRE(main != nullptr);
        ((void(*)())main)();

        // Verify basic while loop - should count to loop_count
        INFO(std::format("loop_count={}, expected={}, actual={}",
                loop_count, loop_count, *o_while_basic));
        REQUIRE(*o_while_basic == loop_count);

        // Verify while with break - should stop at half
        int32_t expected_while_break = loop_count / 2;
        INFO(std::format("loop_count={}, expected={}, actual={}",
                loop_count, expected_while_break, *o_while_break));
        REQUIRE(*o_while_break == expected_while_break);

        // Verify while with continue - sum of even numbers from 0 to loop_count-1
        int32_t expected_while_continue = 0;
        for (int32_t i = 0; i < loop_count; i++) {
            if (i % 2 == 0) {
                expected_while_continue += i;
            }
        }
        INFO(std::format("loop_count={}, expected={}, actual={}",
                loop_count, expected_while_continue, *o_while_continue));
        REQUIRE(*o_while_continue == expected_while_continue);
    }
}


TEST_CASE("For Loops", "[Frontend][ControlFlow]") {
    VCL::ExecutionSession session{};
    session.EnableGDBListener();
    REQUIRE(session.SubmitModule(MakeModule("VCL/controlflow.vcl")));

    SECTION("Value Check") {
        // Generate random loop count
        int32_t loop_count = GENERATE(Catch::Generators::take(1,
                Catch::Generators::random(5, 20)));
        int32_t condition_value = 5;  // Not used for for tests, but required by VCL

        // Define input symbols
        REQUIRE(session.DefineSymbolPtr("condition_value", &condition_value));
        REQUIRE(session.DefineSymbolPtr("loop_count", &loop_count));

        // Lookup output symbols
        int32_t* o_for_basic = (int32_t*)session.Lookup("o_for_basic");
        int32_t* o_for_break = (int32_t*)session.Lookup("o_for_break");
        int32_t* o_for_continue = (int32_t*)session.Lookup("o_for_continue");
        int32_t* o_for_nested = (int32_t*)session.Lookup("o_for_nested");

        // Verify all pointers are valid
        REQUIRE(o_for_basic != nullptr);
        REQUIRE(o_for_break != nullptr);
        REQUIRE(o_for_continue != nullptr);
        REQUIRE(o_for_nested != nullptr);

        // Execute the Main function
        void* main = session.Lookup("Main");
        REQUIRE(main != nullptr);
        ((void(*)())main)();

        // Verify basic for loop - should count to loop_count
        INFO(std::format("loop_count={}, expected={}, actual={}",
                loop_count, loop_count, *o_for_basic));
        REQUIRE(*o_for_basic == loop_count);

        // Verify for with break - should stop at half
        int32_t expected_for_break = loop_count / 2;
        INFO(std::format("loop_count={}, expected={}, actual={}",
                loop_count, expected_for_break, *o_for_break));
        REQUIRE(*o_for_break == expected_for_break);

        // Verify for with continue - sum of odd numbers from 0 to loop_count-1
        int32_t expected_for_continue = 0;
        for (int32_t i = 0; i < loop_count; i++) {
            if (i % 2 != 0) {
                expected_for_continue += i;
            }
        }
        INFO(std::format("loop_count={}, expected={}, actual={}",
                loop_count, expected_for_continue, *o_for_continue));
        REQUIRE(*o_for_continue == expected_for_continue);

        // Verify nested for loops - 3 x 4 = 12 iterations
        int32_t expected_for_nested = 3 * 4;
        INFO(std::format("expected={}, actual={}", expected_for_nested, *o_for_nested));
        REQUIRE(*o_for_nested == expected_for_nested);
    }
}


TEST_CASE("Break and Continue", "[Frontend][ControlFlow]") {
    VCL::ExecutionSession session{};
    session.EnableGDBListener();
    REQUIRE(session.SubmitModule(MakeModule("VCL/controlflow.vcl")));

    SECTION("Break Behavior") {
        // Test that break properly exits loops early
        int32_t loop_count = 10;
        int32_t condition_value = 5;

        REQUIRE(session.DefineSymbolPtr("condition_value", &condition_value));
        REQUIRE(session.DefineSymbolPtr("loop_count", &loop_count));

        int32_t* o_while_break = (int32_t*)session.Lookup("o_while_break");
        int32_t* o_for_break = (int32_t*)session.Lookup("o_for_break");

        REQUIRE(o_while_break != nullptr);
        REQUIRE(o_for_break != nullptr);

        void* main = session.Lookup("Main");
        REQUIRE(main != nullptr);
        ((void(*)())main)();

        // Both should break at loop_count / 2 = 5
        REQUIRE(*o_while_break == 5);
        REQUIRE(*o_for_break == 5);
    }

    SECTION("Continue Behavior") {
        // Test that continue properly skips iterations
        int32_t loop_count = 10;  // 0-9
        int32_t condition_value = 5;

        REQUIRE(session.DefineSymbolPtr("condition_value", &condition_value));
        REQUIRE(session.DefineSymbolPtr("loop_count", &loop_count));

        int32_t* o_while_continue = (int32_t*)session.Lookup("o_while_continue");
        int32_t* o_for_continue = (int32_t*)session.Lookup("o_for_continue");

        REQUIRE(o_while_continue != nullptr);
        REQUIRE(o_for_continue != nullptr);

        void* main = session.Lookup("Main");
        REQUIRE(main != nullptr);
        ((void(*)())main)();

        // While: sum of even numbers 0,2,4,6,8 = 20
        REQUIRE(*o_while_continue == 20);

        // For: sum of odd numbers 1,3,5,7,9 = 25
        REQUIRE(*o_for_continue == 25);
    }
}


TEST_CASE("Complex Control Flow", "[Frontend][ControlFlow]") {
    VCL::ExecutionSession session{};
    session.EnableGDBListener();
    REQUIRE(session.SubmitModule(MakeModule("VCL/controlflow.vcl")));

    SECTION("Combined Test") {
        // Test with various input combinations
        int32_t condition_value = GENERATE(-5, 0, 3, 15);
        int32_t loop_count = GENERATE(8, 12, 16);

        REQUIRE(session.DefineSymbolPtr("condition_value", &condition_value));
        REQUIRE(session.DefineSymbolPtr("loop_count", &loop_count));

        // Execute
        void* main = session.Lookup("Main");
        REQUIRE(main != nullptr);
        ((void(*)())main)();

        // Verify at least one output from each category
        int32_t* o_if_simple = (int32_t*)session.Lookup("o_if_simple");
        int32_t* o_while_basic = (int32_t*)session.Lookup("o_while_basic");
        int32_t* o_for_basic = (int32_t*)session.Lookup("o_for_basic");

        REQUIRE(o_if_simple != nullptr);
        REQUIRE(o_while_basic != nullptr);
        REQUIRE(o_for_basic != nullptr);

        // Verify if statement
        REQUIRE(*o_if_simple == ((condition_value > 0) ? 1 : 0));

        // Verify while and for loops give same count
        REQUIRE(*o_while_basic == loop_count);
        REQUIRE(*o_for_basic == loop_count);
    }
}
