#pragma once


#include <VCL/Core/Diagnostic.hpp>
#include <VCL/Core/DiagnosticConsumer.hpp>
#include <VCL/Core/SourceManager.hpp>
#include <VCL/Core/Identifier.hpp>


namespace VCL {

    class CompilerContext {
    public:
        CompilerContext();
        CompilerContext(const CompilerContext& other) = delete;
        CompilerContext(CompilerContext&& other) = delete;
        ~CompilerContext() = default;

        CompilerContext& operator=(const CompilerContext& other) = delete;
        CompilerContext& operator=(CompilerContext&& other) = delete;

        inline DiagnosticsEngine& GetDiagnosticEngine() { return diagnosticEngine; }
        inline DiagnosticReporter& GetDiagnosticReporter() { return diagnosticReporter; }
        inline DiagnosticConsumer* GetDiagnosticConsumer() const { return diagnosticEngine.GetDiagnosticConsumer(); }
        inline void SetDiagnosticConsumer(DiagnosticConsumer* consumer) { 
            consumer->SetSourceManager(&sourceManager);
            diagnosticEngine.SetDiagnosticConsumer(consumer); 
        }

        inline SourceManager& GetSourceManager() { return sourceManager; }
        
        inline IdentifierTable& GetIdentifierTable() { return identifierTable; }

    private:
        DiagnosticsEngine diagnosticEngine;
        DiagnosticReporter diagnosticReporter;
        SourceManager sourceManager;
        IdentifierTable identifierTable;
    };

}