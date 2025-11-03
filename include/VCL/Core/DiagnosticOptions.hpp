#pragma once


namespace VCL {
    class DiagnosticConsumer;

    class DiagnosticOptions {
    public:
        DiagnosticOptions() = default;
        DiagnosticOptions(const DiagnosticOptions& other) = default;
        DiagnosticOptions(DiagnosticOptions&& other) = default;
        ~DiagnosticOptions() = default;

        DiagnosticOptions& operator=(const DiagnosticOptions& other) = default;
        DiagnosticOptions& operator=(DiagnosticOptions&& other) = default;

        inline DiagnosticConsumer* GetDiagnosticConsumer() { return consumer; }
        inline void SetDiagnosticConsumer(DiagnosticConsumer* consumer) { this->consumer = consumer; }

        inline bool HasTreatWarningAsErrorFlag() const { return flags.treatWarningAsError; }
        inline void SetTreatWarningAsErrorFlag(bool v) { flags.treatWarningAsError = v; }

        inline bool HasIgnoreAllWarningFlag() const { return flags.ignoreAllWarning; }
        inline void SetIgnoreAllWarningFlag(bool v) { flags.ignoreAllWarning = v; }

        inline bool HasIgnoreAllRemarkFlag() const { return flags.ignoreAllRemark; }
        inline void SetIgnoreAllRemarkFlag(bool v) { flags.ignoreAllRemark = v; }

        inline bool HasIgnoreAllNoteFlag() const { return flags.ignoreAllNote; }
        inline void SetIgnoreAllNoteFlag(bool v) { flags.ignoreAllNote = v; }

    private:
        DiagnosticConsumer* consumer = nullptr;

        struct DiagnosticFlags {
            unsigned treatWarningAsError : 1 = 0;
            unsigned ignoreAllWarning : 1 = 0;
            unsigned ignoreAllRemark : 1 = 0;
            unsigned ignoreAllNote : 1 = 0;
        } flags;
    };

}