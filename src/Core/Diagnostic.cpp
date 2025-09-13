#include <VCL/Core/Diagnostic.hpp>

#include <VCL/Core/DiagnosticConsumer.hpp>


bool VCL::DiagnosticsEngine::Diagnose(Diagnostic&& diagnostic) {
    diagnostic.SetSeverity(BumpSeverityLevelIfNeeded(diagnostic.GetSeverity()));
    bool isError = diagnostic.GetSeverity() == Diagnostic::SeverityLevel::Error;
    if (consumer)
        consumer->HandleDiagnostic(std::move(diagnostic));
    return isError;
}

VCL::Diagnostic::SeverityLevel VCL::DiagnosticsEngine::BumpSeverityLevelIfNeeded(Diagnostic::SeverityLevel level) {
    if (level == Diagnostic::SeverityLevel::Warning && treatWarningAsError)
        return Diagnostic::SeverityLevel::Error;
    if (level == Diagnostic::SeverityLevel::Warning && ignoreAllWarning)
        return Diagnostic::SeverityLevel::Ignored;
    if (level == Diagnostic::SeverityLevel::Remark && ignoreAllRemark)
        return Diagnostic::SeverityLevel::Ignored;
    if (level == Diagnostic::SeverityLevel::Note && ignoreAllNote)
        return Diagnostic::SeverityLevel::Ignored;
    return level;
}

bool VCL::DiagnosticReporter::ReportHandle::Report() {
    if (reporter->supressAll)
        return false;
    return reporter->engine.Diagnose(std::move(diag));
}