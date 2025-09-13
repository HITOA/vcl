#include <VCL/Core/CompilerContext.hpp>



VCL::CompilerContext::CompilerContext() : diagnosticEngine{ nullptr }, 
    diagnosticReporter{ diagnosticEngine }, sourceManager{ diagnosticReporter }, identifierTable{} {}