#pragma once

#include <VCL/Core/DiagnosticConsumer.hpp>

#include <VCL/Core/Diagnostic.hpp>
#include <VCL/Core/SourceLocation.hpp>
#include <VCL/Core/Source.hpp>
#include <VCL/Core/SourceManager.hpp>

#include <string>


namespace VCL {

    class TextDiagnosticConsumer : public DiagnosticConsumer {
    public:
        void HandleDiagnostic(Diagnostic&& diagnostic) override;

        virtual void HandleTextDiagnostic(Diagnostic&& diagnostic, const std::string& msg);

    private:
        std::string GetSourceRangeAsDetailedString(Source* source, SourceRange range);
    };

}