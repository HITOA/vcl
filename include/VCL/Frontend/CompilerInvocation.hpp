#pragma once

#include <VCL/Core/DiagnosticOptions.hpp>
#include <VCL/Core/TargetOptions.hpp>

#include <memory>


namespace VCL {

    class CompilerInvocation {
    public:
        CompilerInvocation();
        CompilerInvocation(const CompilerInvocation& other) = default;
        CompilerInvocation(CompilerInvocation&& other) = default;
        ~CompilerInvocation() = default;

        CompilerInvocation& operator=(const CompilerInvocation& other) = default;
        CompilerInvocation& operator=(CompilerInvocation&& other) = default;

        inline DiagnosticOptions& GetDiagnosticOptions() { return *diagnosticOptions; }
        inline TargetOptions& GetTargetOptions() { return *targetOptions; }
 
    private:
        std::shared_ptr<DiagnosticOptions> diagnosticOptions;
        std::shared_ptr<TargetOptions> targetOptions;
    };

}