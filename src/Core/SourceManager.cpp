#include <VCL/Core/SourceManager.hpp>



VCL::Source* VCL::SourceManager::LoadFromDisk(llvm::StringRef filename) {
    llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> r = llvm::MemoryBuffer::getFile(filename, true, true);
    if (!r) {
        reporter.Error(Diagnostic::FileNotFound, filename.str()).SetCompilerInfo(__FILE__, __func__, __LINE__).Report();
        return nullptr;
    }
    return &sources.emplace_back(std::move(r.get()));
}

VCL::Source* VCL::SourceManager::LoadFromMemory(llvm::StringRef buffer, llvm::StringRef name) {
    llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> r = llvm::MemoryBuffer::getMemBuffer(buffer, name, true);
    if (!r) {
        reporter.Error(Diagnostic::MemoryBufferCreationFailed).SetCompilerInfo(__FILE__, __func__, __LINE__).Report();
        return nullptr;
    }
    return &sources.emplace_back(std::move(r.get()));
}

VCL::Source* VCL::SourceManager::GetSourceFromLocation(SourceLocation location) {
    for (size_t i = 0; i < sources.size(); ++i)
        if (sources[i].ContainLocation(location))
            return &sources[i];
    return nullptr;
}