#include <VCL/Core/SourceManager.hpp>

#include <llvm/ADT/SmallString.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>

#include <iostream>
VCL::Source* VCL::SourceManager::LoadFromDisk(llvm::StringRef filename) {
    llvm::SmallString<128> realPath{};
    if (std::error_code code = llvm::sys::fs::real_path(filename, realPath)) {
        reporter.Error(Diagnostic::FileNotFound, filename.str()).SetCompilerInfo(__FILE__, __func__, __LINE__).Report();
        return nullptr;
    }

    if (sources.count(realPath))
        return sources[realPath];

    llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> r = llvm::MemoryBuffer::getFile(realPath, true, true);
    if (!r) {
        reporter.Error(Diagnostic::FileNotFound, realPath.str().str()).SetCompilerInfo(__FILE__, __func__, __LINE__).Report();
        return nullptr;
    }

    Source* source = sources.getAllocator().Allocate<Source>(1);
    new (source) Source{ std::move(r.get()) };
    sources.insert({ realPath, source });
    return source;
}

VCL::Source* VCL::SourceManager::LoadFromMemory(llvm::StringRef buffer, llvm::StringRef name) {
    if (sources.count(name))
        return sources[name];

    llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> r = llvm::MemoryBuffer::getMemBuffer(buffer, name, true);
    if (!r) {
        reporter.Error(Diagnostic::MemoryBufferCreationFailed).SetCompilerInfo(__FILE__, __func__, __LINE__).Report();
        return nullptr;
    }

    Source* source = sources.getAllocator().Allocate<Source>(1);
    new (source) Source{ std::move(r.get()) };
    sources.insert({ name, source });
    return source;
}

VCL::Source* VCL::SourceManager::GetSourceFromLocation(SourceLocation location) {
    if (location.GetID() == 0)
        return nullptr;
    for (auto& source : sources)
        if (source.second->ContainLocation(location))
            return source.second;
    return nullptr;
}