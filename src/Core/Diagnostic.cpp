#include <VCL/Core/Diagnostic.hpp>



bool VCL::DiagnosticsEngine::Diagnose(Diagnostic&& diagnostic) {
    return false;
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