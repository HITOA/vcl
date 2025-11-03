#pragma once

#include <VCL/Core/SourceLocation.hpp>
#include <VCL/Core/DiagnosticOptions.hpp>

#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/IntrusiveRefCntPtr.h>


namespace VCL {

    class DiagnosticConsumer;

    /**
     * Provide positional hint for a diagnostic with a SourceRange 
     * and a SourceLocation.
     */
    class DiagnosticHint {
    public:
        enum DiagnosticHintMsg : unsigned short {
            #define DIAGNOSTIC_HINT(x, y) x,
            #include <VCL/Core/Diagnostic.def>
            NoHintMsg,
            Max
        };

        static inline const char* ToString(DiagnosticHintMsg error) {
            switch (error) {
                #define DIAGNOSTIC_HINT(x, y) case DiagnosticHintMsg::x: return y;
                #include <VCL/Core/Diagnostic.def>
                default: return "Unknown hint";
            }
        }

    public:
        DiagnosticHint() = delete;
        DiagnosticHint(SourceRange range) : range{ range }, msg{ NoHintMsg } {}
        DiagnosticHint(SourceRange range, DiagnosticHintMsg msg) : range{ range }, msg{ msg } {}
        DiagnosticHint(const DiagnosticHint& other) = default;
        DiagnosticHint(DiagnosticHint&& other) = default;
        ~DiagnosticHint() = default;

        DiagnosticHint& operator=(const DiagnosticHint& other) = default;
        DiagnosticHint& operator=(DiagnosticHint&& other) = default;

        inline SourceRange GetRange() const { return range; }
        inline DiagnosticHintMsg GetMessage() const { return msg; }

    private:
        SourceRange range;
        DiagnosticHintMsg msg;
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

        static inline const char* ToString(SeverityLevel error) {
            switch (error) {
                case SeverityLevel::Ignored:    return "ignored";
                case SeverityLevel::Note:       return "note";
                case SeverityLevel::Remark:     return "remark";
                case SeverityLevel::Warning:    return "warning";
                case SeverityLevel::Error:      return "error";
                default: return "unknown severity level";
            }
        }

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
                default: return "Unknown diagnostic";
            }
        }


    public:
        Diagnostic() = delete;
        Diagnostic(SeverityLevel level, DiagnosticMsg msg) : severity{ level }, msg{ msg } {}
        Diagnostic(SeverityLevel level, DiagnosticMsg msg, const char* compilerFile, const char* compilerFunc, uint32_t compilerLine) : 
            severity{ level }, msg{ msg }, compilerFile{ compilerFile }, compilerFunc{ compilerFunc }, compilerLine{ compilerLine } {}
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

        inline DiagnosticMsg GetMessage() const { return msg; }

        inline const char* GetCompilerFile() const { return compilerFile; }
        inline void SetCompilerFile(const char* compilerFile) { this->compilerFile = compilerFile; }

        inline const char* GetCompilerFunc() const { return compilerFunc; }
        inline void SetCompilerFunc(const char* compilerFunc) { this->compilerFunc = compilerFunc; }

        inline uint32_t GetCompilerLine() const { return compilerLine; }
        inline void SetCompilerLine(uint32_t compilerLine) { this->compilerLine = compilerLine; }

        inline llvm::SmallVector<std::string>::iterator GetArgsBegin() { return arguments.begin(); }
        inline llvm::SmallVector<std::string>::iterator GetArgsEnd() { return arguments.end(); }

        inline llvm::SmallVector<DiagnosticHint>::iterator GetHintsBegin() { return hints.begin(); }
        inline llvm::SmallVector<DiagnosticHint>::iterator GetHintsEnd() { return hints.end(); }

    private:
        SeverityLevel severity;
        DiagnosticMsg msg;
        llvm::SmallVector<std::string> arguments{};
        llvm::SmallVector<DiagnosticHint> hints{};

        const char* compilerFile = nullptr;
        const char* compilerFunc = nullptr;
        uint32_t compilerLine = 0;
    };

    /**
     * This is the DiagnosticEngine, it will receive all Diagnostic, 
     * bump them if necessary, and send it to the consumer if needed.
     */
    class DiagnosticsEngine : public llvm::RefCountedBase<DiagnosticsEngine> {
    public:
        DiagnosticsEngine() = delete;
        DiagnosticsEngine(DiagnosticOptions& options) : options{ options } {};
        DiagnosticsEngine(const DiagnosticsEngine& other) = delete;
        DiagnosticsEngine(DiagnosticsEngine&& other) = delete;
        ~DiagnosticsEngine() = default;

        DiagnosticsEngine& operator=(const DiagnosticsEngine& other) = delete;
        DiagnosticsEngine& operator=(DiagnosticsEngine&& other) = delete;

        inline DiagnosticOptions& GetDiagnosticOptions() const { return options; }

        /**
         * Process a diagnostic and return false if it should interrupt execution
         */
        bool Diagnose(Diagnostic&& diagnostic);
    
    private:
        Diagnostic::SeverityLevel BumpSeverityLevelIfNeeded(Diagnostic::SeverityLevel level);

    private:
        DiagnosticOptions& options;
    };

    class DiagnosticReporter : public llvm::RefCountedBase<DiagnosticReporter> {
    public:
        class ReportHandle {
        public:
            ReportHandle() = delete;
            ReportHandle(Diagnostic&& diag, DiagnosticReporter* reporter) : diag{ std::move(diag) }, reporter{ reporter } {}
            ReportHandle(const ReportHandle& other) = delete;
            ReportHandle(ReportHandle&& other) = delete;
            ~ReportHandle() = default;

            ReportHandle& operator=(const ReportHandle& other) = delete;
            ReportHandle& operator=(ReportHandle&& other) = delete;

            inline ReportHandle& AddArgument(const std::string& arg) { 
                diag.AddArgument(arg);
                return *this;
            }

            inline ReportHandle& AddHint(const DiagnosticHint& hint) { 
                diag.AddHints(hint);
                return *this;
            }

            inline ReportHandle& SetCompilerInfo(const char* compilerFile, const char* compilerFunc, uint32_t compilerLine) {
                diag.SetCompilerFile(compilerFile);
                diag.SetCompilerFunc(compilerFunc);
                diag.SetCompilerLine(compilerLine);
                return *this;
            }

            bool Report();
            
        private:
            Diagnostic diag;
            DiagnosticReporter* reporter;
        };
    public:
        DiagnosticReporter() = delete;
        DiagnosticReporter(DiagnosticsEngine& engine) : engine{ engine } {}
        DiagnosticReporter(const DiagnosticReporter& other) = default;
        DiagnosticReporter(DiagnosticReporter&& other) = default;
        ~DiagnosticReporter() = default;

        DiagnosticReporter& operator=(const DiagnosticReporter& other) = default;
        DiagnosticReporter& operator=(DiagnosticReporter&& other) = default;

        inline void SetSupressAll(bool enable) { supressAll = enable; }
        inline bool GetSupressAll() const { return supressAll; }

        template<typename... Args>
        inline ReportHandle Build(Diagnostic::SeverityLevel severity, Diagnostic::DiagnosticMsg msg, Args&&... args) {
            Diagnostic diag{ severity, msg };
            (diag.AddArgument(args), ...);
            return ReportHandle{ std::move(diag), this };
        }

        template<typename... Args>
        inline ReportHandle Error(Diagnostic::DiagnosticMsg msg, Args&&... args) {
            return Build(Diagnostic::SeverityLevel::Error, msg, std::forward<Args>(args)...);
        }
        
        template<typename... Args>
        inline ReportHandle Warn(Diagnostic::DiagnosticMsg msg, Args&&... args) {
            return Build(Diagnostic::SeverityLevel::Warning, msg, std::forward<Args>(args)...);
        }
        
        template<typename... Args>
        inline ReportHandle Remark(Diagnostic::DiagnosticMsg msg, Args&&... args) {
            return Build(Diagnostic::SeverityLevel::Remark, msg, std::forward<Args>(args)...);
        }

        template<typename... Args>
        inline ReportHandle Note(Diagnostic::DiagnosticMsg msg, Args&&... args) {
            return Build(Diagnostic::SeverityLevel::Note, msg, std::forward<Args>(args)...);
        }
        
    private:
        DiagnosticsEngine& engine;
        bool supressAll = false;
    };

}