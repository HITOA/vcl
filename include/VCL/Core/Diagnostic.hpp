#pragma once

#include <VCL/Core/DiagnosticConsumer.hpp>
#include <VCL/Core/SourceLocation.hpp>

#include <llvm/ADT/SmallVector.h>


namespace VCL {

    /**
     * Provide positional hint for a diagnostic with a SourceRange 
     * and a SourceLocation.
     */
    class DiagnosticHint {
    public:
        DiagnosticHint() = delete;
        DiagnosticHint(SourceRange range, SourceLocation location) : range{ range }, location{ location } {}
        DiagnosticHint(const DiagnosticHint& other) = default;
        DiagnosticHint(DiagnosticHint&& other) = default;
        ~DiagnosticHint() = default;

        DiagnosticHint& operator=(const DiagnosticHint& other) = default;
        DiagnosticHint& operator=(DiagnosticHint&& other) = default;

        inline SourceRange GetRange() const { return range; }
        inline SourceLocation GetLocation() const { return location; }

    private:
        SourceRange range;
        SourceLocation location;
    };

    /**
     * This is a Diagnostic containing an error, a severity level, 
     * and possibly hint(s) and argument(s).
     */
    class Diagnostic {
    public:
        /**
         * Somple enum for each severity level
         */
        enum class SeverityLevel : unsigned short {
            Ignored,
            Note,
            Remark,
            Warning,
            Error
        }; 

        /**
         * Simple enum for each error a submodule could return
         */
        enum DiagnosticMsg : unsigned short {
            #define DIAGNOSTIC(x, y) x,
            #include <VCL/Core/Diagnostic.def>
            Max
        };

        /**
         * Provide a way to get a proper string for a given Error
         */
        static inline const char* ToString(DiagnosticMsg error) {
            switch (error) {
                #define DIAGNOSTIC(x, y) case DiagnosticMsg::x: return y;
                #include <VCL/Core/Diagnostic.def>
                default: return "Unknown error";
            }
        }


    public:
        Diagnostic() = delete;
        Diagnostic(SeverityLevel level, DiagnosticMsg msg) : severity{ severity }, msg{ msg } {}
        Diagnostic(const Diagnostic& other) = delete;
        Diagnostic(Diagnostic&& other) = default;
        ~Diagnostic() = default;

        Diagnostic& operator=(const Diagnostic& other) = delete;
        Diagnostic& operator=(Diagnostic&& other) = default;

        /** Add string argument to build the final error message */
        inline void AddArgument(const std::string& arg) { arguments.emplace_back(arg); }
        /** Add hint to give more context to the error message */
        inline void AddHints(const DiagnosticHint& hint) { hints.emplace_back(hint); }
        
        inline SeverityLevel GetSeverity() const { return severity; }
        inline void SetSeverity(SeverityLevel level) { severity = level; }

    private:
        SeverityLevel severity;
        DiagnosticMsg msg;
        llvm::SmallVector<std::string> arguments{};
        llvm::SmallVector<DiagnosticHint> hints{};
    };

    /**
     * This is the DiagnosticEngine, it will receive all Diagnostic, 
     * bump them if necessary, and send it to the consumer if needed.
     */
    class DiagnosticsEngine {
    public:
        DiagnosticsEngine() = default;
        DiagnosticsEngine(DiagnosticConsumer* consumer) : consumer{ consumer } {};
        DiagnosticsEngine(const DiagnosticsEngine& other) = delete;
        DiagnosticsEngine(DiagnosticsEngine&& other) = delete;
        ~DiagnosticsEngine() = default;

        DiagnosticsEngine& operator=(const DiagnosticsEngine& other) = delete;
        DiagnosticsEngine& operator=(DiagnosticsEngine&& other) = delete;

        /**
         * Process a diagnostic and return false if it should interrupt execution
         */
        bool Diagnose(Diagnostic&& diagnostic);

        inline void SetTreatWarningAsError(bool enable) { treatWarningAsError = enable; }
        inline void SetIgnoreAllWarning(bool enable) { ignoreAllWarning = enable; }
        inline void SetIgnoreAllRemark(bool enable) { ignoreAllRemark = enable; }
        inline void SetIgnoreAllNote(bool enable) { ignoreAllNote = enable; }
    
    private:
        Diagnostic::SeverityLevel BumpSeverityLevelIfNeeded(Diagnostic::SeverityLevel level);

    private:
        DiagnosticConsumer* consumer = nullptr;

        bool treatWarningAsError = false;
        bool ignoreAllWarning = false;
        bool ignoreAllRemark = false;
        bool ignoreAllNote = false;
    };

    class DiagnosticReporter {
    public:
        DiagnosticReporter() = delete;
        DiagnosticReporter(DiagnosticsEngine& engine) : engine{ engine } {}
        DiagnosticReporter(const DiagnosticReporter& other) = default;
        DiagnosticReporter(DiagnosticReporter&& other) = default;
        ~DiagnosticReporter() = default;

        DiagnosticReporter& operator=(const DiagnosticReporter& other) = default;
        DiagnosticReporter& operator=(DiagnosticReporter&& other) = default;

        template<typename... Args>
        inline bool Report(Diagnostic::SeverityLevel severity, Diagnostic::DiagnosticMsg msg, Args&&... args) {
            Diagnostic diag{ severity, msg };
            (diag.AddArgument(args), ...);
            return engine.Diagnose(std::move(diag));
        }

        template<typename... Args>
        inline bool Error(Diagnostic::DiagnosticMsg msg, Args&&... args) {
            return Report(Diagnostic::SeverityLevel::Error, msg, std::forward<Args>(args)...);
        }
        
        template<typename... Args>
        inline bool Warn(Diagnostic::DiagnosticMsg msg, Args&&... args) {
            return Report(Diagnostic::SeverityLevel::Warning, msg, std::forward<Args>(args)...);
        }
        
        template<typename... Args>
        inline bool Remark(Diagnostic::DiagnosticMsg msg, Args&&... args) {
            return Report(Diagnostic::SeverityLevel::Remark, msg, std::forward<Args>(args)...);
        }

        template<typename... Args>
        inline bool Note(Diagnostic::DiagnosticMsg msg, Args&&... args) {
            return Report(Diagnostic::SeverityLevel::Note, msg, std::forward<Args>(args)...);
        }
        
    private:
        DiagnosticsEngine& engine;
    };

}