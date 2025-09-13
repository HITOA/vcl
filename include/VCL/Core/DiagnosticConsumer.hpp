#pragma once

#include <VCL/Core/Diagnostic.hpp>
#include <VCL/Core/Source.hpp>
#include <VCL/Core/SourceManager.hpp>

#include <stack>


namespace VCL {

    class DiagnosticConsumer {
    public:
        DiagnosticConsumer() = default;
        virtual ~DiagnosticConsumer() = default;

        virtual void HandleDiagnostic(Diagnostic&& diagnostic) {};

        inline void SetSourceManager(SourceManager* sm) { this->sm = sm; }
        inline SourceManager* GetSourceManager() const { return sm; }

    protected:
        SourceManager* sm;
    };

    class TextDiagnosticConsumer : public DiagnosticConsumer {
    public:
        void HandleDiagnostic(Diagnostic&& diagnostic) override;

        virtual void HandleTextDiagnostic(Diagnostic&& diagnostic, const std::string& msg);

    private:
        std::string GetSourceRangeAsDetailedString(Source* source, SourceRange range);
    };

}