#include <VCL/Core/SourceManager.hpp>



VCL::Source* VCL::SourceManager::LoadFromDisk(llvm::StringRef filename) {
    llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> r = llvm::MemoryBuffer::getFile(filename, true, true);
    if (!r) {
        reporter.Error(Diagnostic::FileNotFound, filename.str());
        return nullptr;
    }
    return &sources.emplace_back(std::move(r.get()));
}

VCL::Source* VCL::SourceManager::LoadFromMemory(llvm::StringRef buffer, llvm::StringRef name) {
    llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> r = llvm::MemoryBuffer::getMemBuffer(buffer, name, true);
    if (!r) {
        reporter.Error(Diagnostic::MemoryBufferCreationFailed);
        return nullptr;
    }
    return &sources.emplace_back(std::move(r.get()));
}