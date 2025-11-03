#include <VCL/Frontend/CompilerInvocation.hpp>


VCL::CompilerInvocation::CompilerInvocation() 
    :   diagnosticOptions{ std::make_shared<DiagnosticOptions>() },
        targetOptions{ std::make_shared<TargetOptions>() }
    {}