#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <VCL/VCL.hpp>
#include <VCL/NativeTarget.hpp>
#include <format>
#include <stdlib.h>


class ConsoleLogger : public VCL::Logger {
public:
    void Log(VCL::Message& message) override {
        const char* severityStr[] = {
            "None",
            "Error",
            "Warning",
            "Info",
            "Debug"
        };
        int severityInt = (int)message.severity;
        INFO(std::format("[%s] %s\n", severityStr[severityInt], message.message.c_str()));
    };
};

#define MAKE_VCL(filename) std::shared_ptr<ConsoleLogger> logger = std::make_shared<ConsoleLogger>(); \
    std::unique_ptr<VCL::Parser> parser = VCL::Parser::Create(logger); \
    std::unique_ptr<VCL::ExecutionSession> session = VCL::ExecutionSession::Create(logger); \
    session->SetDebugInformation(true); \
    std::filesystem::path sourcepath{ filename }; \
    auto source = VCL::Source::LoadFromDisk(sourcepath); \
    REQUIRE(source.has_value()); \
    std::unique_ptr<VCL::ASTProgram> program; \
    REQUIRE_NOTHROW(program = parser->Parse(*source)); \
    std::unique_ptr<VCL::Module> module = session->CreateModule(std::move(program)); \
    VCL::ModuleDebugInformationSettings diSettings{}; \
    diSettings.generateDebugInformation = true; \
    REQUIRE_NOTHROW(module->Emit(diSettings)); \
    REQUIRE_NOTHROW(module->Verify()); \
    session->SubmitModule(std::move(module))

TEST_CASE( "VCL parsing & emit empty file", "[IR][Parsing]" ) {
    MAKE_VCL("./vcl/empty.vcl");
}

TEST_CASE( "VCL parsing & emit & compiling", "[IR][Compiling][Parsing]" ) {
    MAKE_VCL("./vcl/dummy.vcl");
}

TEST_CASE( "VCL input & output", "[Assignment][Binding]" ) {
    MAKE_VCL("./vcl/inout.vcl");
    
    uint32_t vectorElementWidth = VCL::NativeTarget::GetInstance()->GetMaxVectorElementWidth();
    uint32_t vectorAlignment = VCL::NativeTarget::GetInstance()->GetMaxVectorByteWidth();

    SECTION("Value check") {
        float inputFloat = GENERATE(12.0f, 0.0f, -23.0f);
        int inputInt = GENERATE(23, 0, -34);
        bool inputBool = GENERATE(true, false);

        float* inputVFloat = (float*)aligned_alloc(vectorAlignment, sizeof(float) * vectorElementWidth);
        int* inputVInt = (int*)aligned_alloc(vectorAlignment, sizeof(int) * vectorElementWidth);


        float outputFloat = 0.0;
        int outputInt = 0;
        bool outputBool = true;

        float* outputVFloat = (float*)aligned_alloc(vectorAlignment, sizeof(float) * vectorElementWidth);
        int* outputVInt = (int*)aligned_alloc(vectorAlignment, sizeof(int) * vectorElementWidth);

        for (size_t i = 0; i < vectorElementWidth; ++i) {
            inputVFloat[i] = rand();
            inputVInt[i] = rand();
        }

        session->DefineExternSymbolPtr("inputFloat", &inputFloat);
        session->DefineExternSymbolPtr("inputInt", &inputInt);
        session->DefineExternSymbolPtr("inputBool", &inputBool);
        session->DefineExternSymbolPtr("inputVFloat", inputVFloat);
        session->DefineExternSymbolPtr("inputVInt", inputVInt);

        session->DefineExternSymbolPtr("outputFloat", &outputFloat);
        session->DefineExternSymbolPtr("outputInt", &outputInt);
        session->DefineExternSymbolPtr("outputBool", &outputBool);
        session->DefineExternSymbolPtr("outputVFloat", outputVFloat);
        session->DefineExternSymbolPtr("outputVInt", outputVInt);

        ((void(*)())(session->Lookup("Main")))();

        REQUIRE(inputFloat == outputFloat);
        REQUIRE(inputInt == outputInt);
        REQUIRE(inputBool == outputBool);

        for (size_t i = 0; i < vectorElementWidth; ++i) {
            REQUIRE(inputVFloat[i] == outputVFloat[i]);
            REQUIRE(inputVInt[i] == outputVInt[i]);
        }

        free(inputVFloat);
        free(inputVInt);
        free(outputVFloat);
        free(outputVInt);
    }
}

int Factorial(int n) {
    if (n == 0 || n == 1)
        return 1;
    return n * Factorial(n - 1);
}

TEST_CASE( "VCL factorial", "[Condition][Branching][Recursion][Arithmetic]" ) {
    MAKE_VCL("./vcl/factorial.vcl");

    SECTION("Value check") {
        int input = GENERATE(0, 1, 5, 3, 10);
        int output = 0;

        session->DefineExternSymbolPtr("input", &input);
        session->DefineExternSymbolPtr("output", &output);

        ((void(*)())(session->Lookup("Main")))();
        
        REQUIRE(Factorial(input) == output);
    }
}


TEST_CASE( "VCL infinite recursion with select detection", "[Error][Detection][Recursion]" ) {
    std::shared_ptr<ConsoleLogger> logger = std::make_shared<ConsoleLogger>();
    std::unique_ptr<VCL::Parser> parser = VCL::Parser::Create(logger);
    std::unique_ptr<VCL::ExecutionSession> session = VCL::ExecutionSession::Create(logger);
    session->SetDebugInformation(true);
    std::filesystem::path sourcepath{ "./vcl/infiniterec.vcl" };
    auto source = VCL::Source::LoadFromDisk(sourcepath);
    REQUIRE(source.has_value());
    std::unique_ptr<VCL::ASTProgram> program;
    REQUIRE_NOTHROW(program = parser->Parse(*source));
    std::unique_ptr<VCL::Module> module = session->CreateModule(std::move(program));
    REQUIRE_NOTHROW(module->Emit());
    VCL::ModuleVerifierSettings settings{};
    settings.selectRecursionAsError = true;
    REQUIRE_THROWS(module->Verify(settings));
}