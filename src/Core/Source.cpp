#include <VCL/Core/Source.hpp>

#include <fstream>
#include <algorithm>


std::pair<uint32_t, uint32_t> VCL::Source::GetLineColumn(uint32_t offset) const {
    auto it = std::upper_bound(lineOffsets.begin(), lineOffsets.end(), offset);
    uint32_t line = std::distance(lineOffsets.begin(), it);
    return std::make_pair(line, offset - line);
}

std::expected<VCL::Handle<VCL::Source>, VCL::SourceError> VCL::Source::LoadFromDisk(llvm::StringRef path) {
    llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> r = llvm::MemoryBuffer::getFile(path, true, true);
    if (!r)
        return std::unexpected{ SourceError::FileNotFound };

    Handle<Source> source = MakeHandle<Source>();
    source->buffer = std::move(r.get());
    source->CalculateLineOffsets();
    return source;
}

std::expected<VCL::Handle<VCL::Source>, VCL::SourceError> VCL::Source::LoadFromMemory(llvm::StringRef buffer, llvm::StringRef name) {
    llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> r = llvm::MemoryBuffer::getMemBuffer(buffer, name, true);
    if (!r)
        return std::unexpected{ SourceError::MemoryBufferCreationFailed };

    Handle<Source> source = MakeHandle<Source>();
    source->buffer = std::move(r.get());
    source->CalculateLineOffsets();
    return source;
}

void VCL::Source::CalculateLineOffsets() {
    const char* bufferBegin = buffer->getBufferStart();
    lineOffsets.clear();
    lineOffsets.push_back(0);
    for (uint32_t offset = 0; offset < buffer->getBufferSize(); ++offset)
        if (bufferBegin[offset] == '\n')
            lineOffsets.push_back(offset);
}