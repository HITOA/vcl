#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <VCL/Parser.hpp>
#include <VCL/ExecutionSession.hpp>
#include <VCL/Module.hpp>
#include <VCL/NativeTarget.hpp>
#include <VCL/Error.hpp>
#include <format>
#include <stdlib.h>
#include <iostream>


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
        if (severityInt == 1) {
            CAPTURE(std::format("[%s] %s\n", severityStr[severityInt], message.message.c_str()));
        }
        else if (severityInt == 2) {
            WARN(std::format("[%s] %s\n", severityStr[severityInt], message.message.c_str()));
        }
        else {
            INFO(std::format("[%s] %s\n", severityStr[severityInt], message.message.c_str()));
        }
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
    logger->Error("DAMN UWU");
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

TEST_CASE( "VCL const test", "[Error]" ) {
    std::shared_ptr<ConsoleLogger> logger = std::make_shared<ConsoleLogger>();
    std::unique_ptr<VCL::Parser> parser = VCL::Parser::Create(logger);
    std::unique_ptr<VCL::ExecutionSession> session = VCL::ExecutionSession::Create(logger);
    session->SetDebugInformation(true);
    std::filesystem::path sourcepath{ "./vcl/const.vcl" };
    auto source = VCL::Source::LoadFromDisk(sourcepath);
    REQUIRE(source.has_value());
    std::unique_ptr<VCL::ASTProgram> program;
    REQUIRE_NOTHROW(program = parser->Parse(*source));
    std::unique_ptr<VCL::Module> module = session->CreateModule(std::move(program));
    REQUIRE_THROWS(module->Emit());
}

template<typename T>
T Max(T a, T b) {
    return a > b ? a : b;
}

TEST_CASE( "VCL templated max", "[Template]" ) {
    MAKE_VCL("./vcl/templatefunction.vcl");

    SECTION("Value check") {
        float a = GENERATE(0.0f, 2.0f, -2.0f, 5.0f);
        float b = GENERATE(4.0f, 2.0f, -1.0f, 0.0f);
        
        float r;

        session->DefineExternSymbolPtr("a", &a);
        session->DefineExternSymbolPtr("b", &b);

        session->DefineExternSymbolPtr("r", &r);

        ((void(*)())(session->Lookup("Main")))();
        
        REQUIRE(Max(a, b) == r);
    }
}

template<typename T>
struct Vector3 {
    T x;
    T y;
    T z;
};

template<typename T>
T Distance(Vector3<T> a, Vector3<T> b) {
    return sqrt(pow(b.x - a.x, 2.0) + pow(b.y - a.y, 2.0) + pow(b.z - a.z, 2.0));
}

TEST_CASE( "VCL distance", "[Arithmetic][Intrinsic][Struct][Template]" ) {
    MAKE_VCL("./vcl/distance.vcl");

    SECTION("Value check") {
        Vector3<float> vecA{ 0.0f, 10.0f, 3.0f };
        Vector3<float> vecB{ -2.0f, 0.0f, 4.0f };

        float distance;

        session->DefineExternSymbolPtr("vecA", &vecA);
        session->DefineExternSymbolPtr("vecB", &vecB);

        session->DefineExternSymbolPtr("distance", &distance);

        ((void(*)())(session->Lookup("Main")))();
        
        REQUIRE(Distance(vecA, vecB) == distance);
    }
}

TEST_CASE( "VCL initializer on extern global variable", "[Error]" ) {
    std::shared_ptr<ConsoleLogger> logger = std::make_shared<ConsoleLogger>();
    std::unique_ptr<VCL::Parser> parser = VCL::Parser::Create(logger);
    std::unique_ptr<VCL::ExecutionSession> session = VCL::ExecutionSession::Create(logger);
    session->SetDebugInformation(true);
    std::filesystem::path sourcepath{ "./vcl/initializeronextern.vcl" };
    auto source = VCL::Source::LoadFromDisk(sourcepath);
    REQUIRE(source.has_value());
    std::unique_ptr<VCL::ASTProgram> program;
    REQUIRE_NOTHROW(program = parser->Parse(*source));
    std::unique_ptr<VCL::Module> module = session->CreateModule(std::move(program));
    REQUIRE_THROWS(module->Emit());
}

TEST_CASE( "VCL array", "[Array]" ) {
    MAKE_VCL("./vcl/array.vcl");

    SECTION("Value check") {
        int input = GENERATE(2.0, 5.0, -2.0, 0.0);
        int output = 0;
        
        session->DefineExternSymbolPtr("input", &input);
        session->DefineExternSymbolPtr("output", &output);

        ((void(*)())(session->Lookup("Main")))();

        int finput = input;

        for (int i = 0; i < 7; ++i) {
            input = rand();
            ((void(*)())(session->Lookup("Main")))();
        }
        
        REQUIRE(output == finput);
    }
}

template<typename T>
struct Span {
    T* data;
    int length;
};

TEST_CASE( "VCL span", "[Span]" ) {
    MAKE_VCL("./vcl/span.vcl");

    SECTION("Value check") {
        float buffer[64]{};

        Span<float> inputs{ buffer, 64 };
        int index = 0;

        float output = 0;
        int length = 0;
        
        session->DefineExternSymbolPtr("inputs", &inputs);
        session->DefineExternSymbolPtr("index", &index);

        session->DefineExternSymbolPtr("output", &output);
        session->DefineExternSymbolPtr("length", &length);

        for (int i = 0; i < 64; ++i)
            buffer[i] = rand();

        for (int i = 0; i < 8; ++i) {
            index = (i * 11) % 64;
            ((void(*)())(session->Lookup("Main")))();
            REQUIRE(buffer[index] == output);
            REQUIRE(length == 64);
        }
    }
}


TEST_CASE( "VCL templated ring", "[Array][Template][Struct]" ) {
    MAKE_VCL("./vcl/templatering.vcl");

    uint32_t vectorElementWidth = VCL::NativeTarget::GetInstance()->GetMaxVectorElementWidth();
    uint32_t vectorAlignment = VCL::NativeTarget::GetInstance()->GetMaxVectorByteWidth();

    SECTION("Value check") {
        
        float* firstInput = (float*)aligned_alloc(vectorAlignment, sizeof(float) * vectorElementWidth);
        float* input = (float*)aligned_alloc(vectorAlignment, sizeof(float) * vectorElementWidth);
        int delay = 36;

        float* output = (float*)aligned_alloc(vectorAlignment, sizeof(float) * vectorElementWidth);

        session->DefineExternSymbolPtr("input", input);
        session->DefineExternSymbolPtr("delay", &delay);
        session->DefineExternSymbolPtr("output", output);

        for (size_t i = 0; i < vectorElementWidth; ++i) {
            input[i] = (float)rand() / RAND_MAX;
            firstInput[i] = input[i];
        }

        ((void(*)())(session->Lookup("Main")))();

        for (size_t i = 0; i < delay - 1; ++i) {
            for (size_t i = 0; i < vectorElementWidth; ++i) {
                input[i] = (float)rand() / RAND_MAX;
            }
            ((void(*)())(session->Lookup("Main")))();
        }

        for (size_t i = 0; i < vectorElementWidth; ++i) {
            REQUIRE(firstInput[i] == output[i]);
        }

        free(input);
        free(output);
    }
}

TEST_CASE( "VCL in out parameters", "[Struct][Qualifiers]" ) {
    MAKE_VCL("./vcl/inoutparameters.vcl");

    SECTION("Value check") {
        float a1 = 0.0;
        float a2 = 0.0;
        float a3 = 0.0;
        float b1 = 0.0;
        float b2 = 0.0;
        float b3 = 0.0;

        session->DefineExternSymbolPtr("a1", &a1);
        session->DefineExternSymbolPtr("a2", &a2);
        session->DefineExternSymbolPtr("a3", &a3);
        session->DefineExternSymbolPtr("b1", &b1);
        session->DefineExternSymbolPtr("b2", &b2);
        session->DefineExternSymbolPtr("b3", &b3);

        ((void(*)())(session->Lookup("Main")))();

        REQUIRE(a1 == 0.0);
        REQUIRE(a2 == 0.0);
        REQUIRE(a3 == 1.0);
        REQUIRE(b1 == 1.0);
        REQUIRE(b2 == 0.0);
        REQUIRE(b3 == 1.0);
    }
}

TEST_CASE( "VCL const array init", "[Array][Aggregate]" ) {
    MAKE_VCL("./vcl/constarrayinit.vcl");

    static float values[16] = {
        4.92, 8.84, 9.28, 5.45,
        9.66, 1.53, 9.73, 3.68,
        6.93, 4.51, 5.65, 3.89,
        1.65, 5.23, 8.84, 5.67
    };

    SECTION("Value check") {
        int index = 0;
        float output;

        session->DefineExternSymbolPtr("index", &index);
        session->DefineExternSymbolPtr("output", &output);

        for (int i = 0; i < 16; ++i) {
            index = i;
            ((void(*)())(session->Lookup("Main")))();
            REQUIRE(output == values[i]);
        }
    }
}

TEST_CASE( "VCL const struct init", "[Array][Aggregate]" ) {
    MAKE_VCL("./vcl/conststructinit.vcl");

    SECTION("Value check") {
        float distance;

        session->DefineExternSymbolPtr("distance", &distance);

        ((void(*)())(session->Lookup("Main")))();
        REQUIRE(distance == 3.0f);
    }
}

TEST_CASE( "VCL attribute", "[Attribute]" ) {
    std::shared_ptr<ConsoleLogger> logger = std::make_shared<ConsoleLogger>();
    std::unique_ptr<VCL::Parser> parser = VCL::Parser::Create(logger);
    std::unique_ptr<VCL::ExecutionSession> session = VCL::ExecutionSession::Create(logger);
    session->SetDebugInformation(true);
    std::filesystem::path sourcepath{ "./vcl/attribute.vcl" };
    auto source = VCL::Source::LoadFromDisk(sourcepath);
    REQUIRE(source.has_value());
    std::unique_ptr<VCL::ASTProgram> program;
    REQUIRE_NOTHROW(program = parser->Parse(*source));
    std::unique_ptr<VCL::Module> module = session->CreateModule(std::move(program));
    VCL::ModuleDebugInformationSettings diSettings{};
    diSettings.generateDebugInformation = true;
    REQUIRE_NOTHROW(module->Emit(diSettings));
    std::shared_ptr<VCL::ModuleInfo> moduleInfo = module->GetModuleInfo();
    REQUIRE_NOTHROW(module->Verify());
    session->SubmitModule(std::move(module));

    SECTION("Value check") {
        float v1 = 3.0f;
        float v2 = 1.0f;

        float r = 0.0f;

        session->DefineExternSymbolPtr("v1", &v1);
        session->DefineExternSymbolPtr("v2", &v2);

        session->DefineExternSymbolPtr("r", &r);
        
        std::string entryPointName{};

        for (auto functionInfo : moduleInfo->GetFunctions()) {
            if (functionInfo->attributes.HasAttributeByName("EntryPoint")) {
                entryPointName = functionInfo->name;
                break;
            }
        }

        if (auto f = (void(*)())(session->Lookup(entryPointName)))
            f();

        REQUIRE(r == (v1 + v2));
    }
}

TEST_CASE( "VCL macro", "[Macro]" ) {
    std::shared_ptr<ConsoleLogger> logger = std::make_shared<ConsoleLogger>();
    std::shared_ptr<VCL::DirectiveRegistry> registry = VCL::DirectiveRegistry::Create();
    registry->RegisterDefaultDirectives();
    std::unique_ptr<VCL::Parser> parser = VCL::Parser::Create(logger);
    parser->SetDirectiveRegistry(registry);
    std::unique_ptr<VCL::ExecutionSession> session = VCL::ExecutionSession::Create(logger);
    session->SetDebugInformation(true);
    std::filesystem::path sourcepath{ "./vcl/macro.vcl" };
    auto source = VCL::Source::LoadFromDisk(sourcepath);
    REQUIRE(source.has_value());
    std::unique_ptr<VCL::ASTProgram> program;
    REQUIRE_NOTHROW(program = parser->Parse(*source));
    std::unique_ptr<VCL::Module> module = session->CreateModule(std::move(program));
    VCL::ModuleDebugInformationSettings diSettings{};
    diSettings.generateDebugInformation = true;
    REQUIRE_NOTHROW(module->Emit(diSettings));
    std::shared_ptr<VCL::ModuleInfo> moduleInfo = module->GetModuleInfo();
    REQUIRE_NOTHROW(module->Verify());
    session->SubmitModule(std::move(module));
}

TEST_CASE( "VCL preprocessor", "[Macro][Include]" ) {
    std::shared_ptr<ConsoleLogger> logger = std::make_shared<ConsoleLogger>();
    std::shared_ptr<VCL::DirectiveRegistry> registry = VCL::DirectiveRegistry::Create();
    registry->RegisterDefaultDirectives();
    std::unique_ptr<VCL::Parser> parser = VCL::Parser::Create(logger);
    parser->SetDirectiveRegistry(registry);
    std::unique_ptr<VCL::ExecutionSession> session = VCL::ExecutionSession::Create(logger);
    session->SetDebugInformation(true);
    std::filesystem::path sourcepath{ "./vcl/preprocessor.vcl" };
    auto source = VCL::Source::LoadFromDisk(sourcepath);
    REQUIRE(source.has_value());
    std::unique_ptr<VCL::ASTProgram> program;
    REQUIRE_NOTHROW(program = parser->Parse(*source));
    std::unique_ptr<VCL::Module> module = session->CreateModule(std::move(program));
    VCL::ModuleDebugInformationSettings diSettings{};
    diSettings.generateDebugInformation = true;
    REQUIRE_NOTHROW(module->Emit(diSettings));
    std::shared_ptr<VCL::ModuleInfo> moduleInfo = module->GetModuleInfo();
    REQUIRE_NOTHROW(module->Verify());
    session->SubmitModule(std::move(module));
    
    SECTION("Value check") {
        int output;

        session->DefineExternSymbolPtr("output", &output);

        ((void(*)())(session->Lookup("Main")))();
        REQUIRE(output == 54);
    }
}