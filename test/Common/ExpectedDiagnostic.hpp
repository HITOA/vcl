#pragma once

#include <VCL/Core/Diagnostic.hpp>
#include <VCL/Core/DiagnosticConsumer.hpp>

#include <catch2/catch_test_macros.hpp>


template<VCL::Diagnostic::DiagnosticMsg Message>
class ExpectedDiagnostic : public VCL::TextDiagnosticConsumer {
public:
    void HandleTextDiagnostic(VCL::Diagnostic&& diagnostic, const std::string& msg) override {
        std::string info = "ExpectedDiagnostic Wrong Diagnostic; ";
        INFO(info + msg);
        REQUIRE(diagnostic.GetMessage() == Message);
    }
};

class ExpectedNoDiagnostic : public VCL::TextDiagnosticConsumer {
public:
    void HandleTextDiagnostic(VCL::Diagnostic&& diagnostic, const std::string& msg) override {
        INFO(msg);
        REQUIRE(false);
    }
};