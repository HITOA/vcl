#pragma once


namespace VCL {
    class Diagnostic;
    class SourceManager;

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

}