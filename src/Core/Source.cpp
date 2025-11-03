#include <VCL/Core/Source.hpp>


VCL::Source::Line VCL::Source::GetLine(SourceLocation location) const {
    if (lineOffsets.size() == 1)
        return Line{ 0, (uint32_t)GetBufferRef().getBufferSize() + 1, 1 };
    llvm::MemoryBufferRef buff = GetBufferRef();
    uint32_t offset = location.GetPtr() - GetBufferRef().getBufferStart();
    for (uint32_t i = 0; i < lineOffsets.size(); ++i)
        if (lineOffsets[i] >= offset)
            return Line{ lineOffsets[i - 1], lineOffsets[i] - lineOffsets[i - 1], i };
    return Line{ lineOffsets[lineOffsets.size() - 1], 
        (uint32_t)GetBufferRef().getBufferSize() - lineOffsets[lineOffsets.size() - 1] + 1, (uint32_t)lineOffsets.size() };
}

void VCL::Source::CalculateLineOffsets() {
    lineOffsets.clear();
    lineOffsets.emplace_back(0);
    llvm::MemoryBufferRef buff = GetBufferRef();
    for (uint32_t i = 1; i < buff.getBufferSize(); ++i)
        if (buff.getBuffer()[i - 1] == '\n')
            lineOffsets.emplace_back(i);
}