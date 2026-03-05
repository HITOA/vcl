#include <VCL/Core/Diagnostic.hpp>

#include <VCL/Core/DiagnosticConsumer.hpp>


bool VCL::DiagnosticsEngine::Diagnose(Diagnostic&& diagnostic) {
    if (options.HasIgnoreMissingDirective() && diagnostic.GetMessage() == Diagnostic::DiagnosticMsg::DirectiveDoesNotExist)
        return false;
    diagnostic.SetSeverity(BumpSeverityLevelIfNeeded(diagnostic.GetSeverity()));
    bool isError = diagnostic.GetSeverity() == Diagnostic::SeverityLevel::Error;
    if (options.GetDiagnosticConsumer())
        options.GetDiagnosticConsumer()->HandleDiagnostic(std::move(diagnostic));
    return isError;
}

VCL::Diagnostic::SeverityLevel VCL::DiagnosticsEngine::BumpSeverityLevelIfNeeded(Diagnostic::SeverityLevel level) {
    if (level == Diagnostic::SeverityLevel::Warning && options.HasTreatWarningAsErrorFlag())
        return Diagnostic::SeverityLevel::Error;
    if (level == Diagnostic::SeverityLevel::Warning && options.HasIgnoreAllWarningFlag())
        return Diagnostic::SeverityLevel::Ignored;
    if (level == Diagnostic::SeverityLevel::Remark && options.HasIgnoreAllRemarkFlag())
        return Diagnostic::SeverityLevel::Ignored;
    if (level == Diagnostic::SeverityLevel::Note && options.HasIgnoreAllNoteFlag())
        return Diagnostic::SeverityLevel::Ignored;
    return level;
}

bool VCL::DiagnosticReporter::ReportHandle::Report() {
    if (reporter->supressAll)
        return false;
    return reporter->engine.Diagnose(std::move(diag));
}