#include <VCL/Core/Source.hpp>



VCL::Source::Line VCL::Source::GetLine(SourceLocation location) const {
    llvm::MemoryBufferRef buff = GetBufferRef();
    uint32_t offset = location.GetPtr() - GetBufferRef().getBufferStart();
    for (uint32_t i = 0; i < lineOffsets.size(); ++i)
        if (lineOffsets[i] >= offset)
            return Line{ lineOffsets[i - 1], lineOffsets[i] - lineOffsets[i - 1], i };
    return Line{ lineOffsets[lineOffsets.size() - 1], (uint32_t)lineOffsets.size() };
}

void VCL::Source::CalculateLineOffsets() {
    lineOffsets.clear();
    lineOffsets.emplace_back(0);
    llvm::MemoryBufferRef buff = GetBufferRef();
    for (uint32_t i = 0; i < buff.getBufferSize(); ++i)
        if (buff.getBuffer()[i] == '\n')
            lineOffsets.emplace_back(i);
}