#pragma once

#include <VCL/Core/Diagnostic.hpp>
#include <VCL/Core/DiagnosticConsumer.hpp>
#include <VCL/Frontend/TextDiagnosticConsumer.hpp>

#include <catch2/catch_test_macros.hpp>


template<VCL::Diagnostic::DiagnosticMsg Message>
class ExpectedDiagnostic : public VCL::TextDiagnosticConsumer {
public:
    void HandleTextDiagnostic(VCL::Diagnostic&& diagnostic, const std::string& msg) override {
        std::string info = "ExpectedDiagnostic wrong diagnostic; ";
        INFO(info + msg);
        REQUIRE(diagnostic.GetMessage() == Message);
        ok = true;
    }

    inline void Require() const { 
        INFO("ExpectedDiagnostic no diagnostic");
        REQUIRE(ok); 
    }

private:
    bool ok = false;
};

class ExpectedNoDiagnostic : public VCL::TextDiagnosticConsumer {
public:
    void HandleTextDiagnostic(VCL::Diagnostic&& diagnostic, const std::string& msg) override {
        INFO(msg);
        REQUIRE(false);
    }
};