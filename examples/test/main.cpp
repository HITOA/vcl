#include <vcl/vcl.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <filesystem>

#define SAMPLE_RATE 44100

float* AllocFloatVector() {
    return (float*)aligned_alloc(VCL::GetMaxVectorWidth(), VCL::GetMaxVectorWidth());
}

std::string GetSource(std::filesystem::path filepath) {
    std::ifstream file{ filepath, std::ios::binary };

    if (!file.is_open()) {
        std::cout << "Couldn't open source file \"" << filepath << "\"" << std::endl;
        return "";
    }

    std::stringstream buffer{};
    buffer << file.rdbuf();
    std::string source = buffer.str();
    file.close();

    return source;
}

void TestSquareosc(std::filesystem::path filepath) {
    std::cout << "Testing: " << filepath << std::endl;

    std::string source = GetSource(filepath);
    if (source.empty()) {
        std::cout << "Missing source file for " << filepath << std::endl;
        return;
    }

    try {
        std::shared_ptr<VCL::JITContext> context = VCL::JITContext::Create();
        
        std::unique_ptr<VCL::Module> module = VCL::Module::Create(filepath.filename().c_str(), context);
        module->BindSource(source);
        module->Optimize();

        context->AddModule(std::move(module));

        float fvs = VCL::GetMaxVectorWidth() / sizeof(float);
        
        float* time = AllocFloatVector();
        float frequency = 440.0f;
        float velocity = 1.0;
        float rate = SAMPLE_RATE;

        float* outLeft = AllocFloatVector();
        float* outRight = AllocFloatVector();

        for (int i = 0; i < fvs; ++i)
            time[i] = ((float)i + rate / frequency - fvs / 2) / rate;
        
        context->BindGlobalVariable("time", time);
        context->BindGlobalVariable("frequency", &frequency);
        context->BindGlobalVariable("velocity", &velocity);
        context->BindGlobalVariable("rate", &rate);
        
        context->BindGlobalVariable("outLeft", outLeft);
        context->BindGlobalVariable("outRight", outRight);

        void(*Main)() = (void(*)())context->Lookup("Main");
        Main();
        
        std::cout << "\tLeft: ";
        for (int i = 0; i < fvs; ++i)
            std::cout << outLeft[i] << ", ";
        std::cout << "\n\tRight: ";
        for (int i = 0; i < fvs; ++i)
            std::cout << outRight[i] << ", ";
        std::cout << std::endl;

        free(time);
        free(outLeft);
        free(outRight);

    } catch (std::exception& err) {
        std::cout << "Error while testing: " << filepath << " [" << err.what() << "]" << std::endl;
    }
}

void TestDelay(std::filesystem::path filepath) {
    std::cout << "Testing: " << filepath << std::endl;

    std::string source = GetSource(filepath);
    if (source.empty()) {
        std::cout << "Missing source file for " << filepath << std::endl;
        return;
    }

    try {
        std::shared_ptr<VCL::JITContext> context = VCL::JITContext::Create();
        
        std::unique_ptr<VCL::Module> module = VCL::Module::Create(filepath.filename().c_str(), context);
        module->BindSource(source);
        module->Optimize();

        context->AddModule(std::move(module));

        
        float fvs = VCL::GetMaxVectorWidth() / sizeof(float);
        
        float* inputs = AllocFloatVector();
        float delay = fvs / 2;
        float* outputs = AllocFloatVector();

        for (int i = 0; i < fvs; ++i)
            inputs[i] = i;

        context->BindGlobalVariable("inputs", inputs);
        context->BindGlobalVariable("delay", &delay);
        context->BindGlobalVariable("outputs", outputs);

        void(*Main)() = (void(*)())context->Lookup("Main");
        Main();

        for (int i = 0; i < fvs; ++i)
            inputs[i] = i + fvs;

        Main();
        
        std::cout << "\tOutputs: ";
        for (int i = 0; i < fvs; ++i)
            std::cout << outputs[i] << ", ";
        std::cout << std::endl;

    } catch (std::exception& err) {
        std::cout << "Error while testing: " << filepath << " [" << err.what() << "]" << std::endl;
    }
}

int main(int argc, char** argv) {
    if (argc <= 1)
        return 1;

    std::filesystem::path filepath = argv[1];
    
    TestSquareosc(filepath / "squareosc.vcl");
    TestDelay(filepath / "delay.vcl");

    return 0;
}