#include <VCL/Core/Source.hpp>

#include <fstream>
#include <algorithm>



std::expected<VCL::Handle<VCL::Source>, VCL::SourceError> VCL::Source::LoadFromDisk(llvm::StringRef path) {
    llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> r = llvm::MemoryBuffer::getFile(path, true, true);
    if (!r)
        return std::unexpected{ SourceError::FileNotFound };

    Handle<Source> source = MakeHandle<Source>();
    source->buffer = std::move(r.get());
    return source;
}

std::expected<VCL::Handle<VCL::Source>, VCL::SourceError> VCL::Source::LoadFromMemory(llvm::StringRef buffer, llvm::StringRef name) {
    llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> r = llvm::MemoryBuffer::getMemBuffer(buffer, name, true);
    if (!r)
        return std::unexpected{ SourceError::MemoryBufferCreationFailed };

    Handle<Source> source = MakeHandle<Source>();
    source->buffer = std::move(r.get());
    return source;
}