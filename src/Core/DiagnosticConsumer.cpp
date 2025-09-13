#include <VCL/Core/DiagnosticConsumer.hpp>

#include <VCL/Core/Format.hpp>

#include <iostream>
#include <format>
#include <filesystem>


void VCL::TextDiagnosticConsumer::HandleDiagnostic(Diagnostic&& diagnostic) {
    std::string* argsBegin = diagnostic.GetArgsBegin();
    size_t argsCount = diagnostic.GetArgsEnd() - diagnostic.GetArgsBegin();
    std::string formatedDiag = FormatString(Diagnostic::ToString(diagnostic.GetMessage()), argsBegin, argsCount);
    std::string formatedHints = "";
    const char* severity = Diagnostic::ToString(diagnostic.GetSeverity());
    for (auto it = diagnostic.GetHintsBegin(); it != diagnostic.GetHintsEnd(); ++it) {
        SourceRange range = it->GetRange();
        Source* source = GetSourceManager()->GetSourceFromLocation(range.start);
        Source::Line line = source->GetLine(range.start);
        uint32_t col = (range.start.GetPtr() - source->GetBufferRef().getBufferStart()) - line.offset;
        const char* hintMsg = DiagnosticHint::ToString(it->GetMessage());
        std::string detail = GetSourceRangeAsDetailedString(source, range);
        if (it->GetMessage() == DiagnosticHint::NoHintMsg)
            formatedHints += std::format("{}:{}:{}: {}: {}\n{}\n", source->GetBufferIdentifier().str(), line.lineNumber, col, severity, formatedDiag, detail);
        else
            formatedHints += std::format("{}:{}:{}: hint: {}\n{}\n", source->GetBufferIdentifier().str(), line.lineNumber, col, hintMsg, detail);
    }

    std::string diagMsg = formatedHints.empty() ? formatedDiag : formatedHints;

    if (diagnostic.GetCompilerFile() && diagnostic.GetCompilerFunc()) {
        std::filesystem::path compilerFile{ diagnostic.GetCompilerFile() };
        std::string compilerLocationMsg = std::format("[compiler diagnostic emitted at {}:{} ({})]\n", 
            compilerFile.filename().string(), diagnostic.GetCompilerLine(), diagnostic.GetCompilerFunc());
        diagMsg = compilerLocationMsg + diagMsg;
    }

    HandleTextDiagnostic(std::move(diagnostic), diagMsg);
}

void VCL::TextDiagnosticConsumer::HandleTextDiagnostic(Diagnostic&& diagnostic, const std::string& msg) {
    std::cout << msg << std::endl;
}

std::string VCL::TextDiagnosticConsumer::GetSourceRangeAsDetailedString(Source* source, SourceRange range) {
    Source::Line line = source->GetLine(range.start);
    std::string lineNumberStr = std::to_string(line.lineNumber);
    std::string lineStr = std::format("\t{} |\t{}", lineNumberStr, source->GetBufferRef().getBuffer().substr(line.offset + 1, line.length - 1).str());
    std::string squigleStr = "\t";
    const char* start = source->GetBufferRef().getBufferStart();
    const char* end = source->GetBufferRef().getBufferEnd();
    squigleStr.reserve(lineNumberStr.size());
    for (uint32_t i = 0; i < lineNumberStr.size() + 1; ++i)
        squigleStr += ' ';
    squigleStr += "|\t";
    uint32_t rangeStartOffset = range.start.GetPtr() - start - 1;
    uint32_t rangeEndOffset = range.end.GetPtr() - start - 1;
    for (uint32_t i = 0; i < line.length - 1; ++i) {
        uint32_t offset = i + line.offset;
        if (offset == rangeStartOffset) {
            squigleStr += '^';
        } else if (offset > rangeStartOffset && offset < rangeEndOffset) {
            squigleStr += '~';
        } else {
            squigleStr += ' ';
        }
    }
    return std::format("{}\n{}\n", lineStr, squigleStr);
}
