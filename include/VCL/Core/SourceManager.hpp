#pragma once

#include <VCL/Core/Source.hpp>
#include <VCL/Core/Diagnostic.hpp>

#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/IntrusiveRefCntPtr.h>

#include <expected>
#include <vector>


namespace VCL {

    /**
     * Manage all the sources
     */
    class SourceManager : public llvm::RefCountedBase<SourceManager> {
    public:
        SourceManager() = delete;
        SourceManager(DiagnosticReporter& reporter) : reporter{ reporter } {}
        SourceManager(const SourceManager& other) = delete;
        SourceManager(SourceManager&& other) = delete;
        ~SourceManager() = default;

        SourceManager& operator=(const SourceManager& other) = delete;
        SourceManager& operator=(SourceManager&& other) = delete;

        /** 
         * Load a Source from disk.
         * If this file doesn't exist or cannot be opened, a nullptr will be returned.
         */
        Source* LoadFromDisk(llvm::StringRef filename);
        /**
         * Load a Source from memory from the given buffer.
         * a name can also be given and will be the Source's buffer identifier.
         */
        Source* LoadFromMemory(llvm::StringRef buffer, llvm::StringRef name = "");

        Source* GetSourceFromLocation(SourceLocation location);
        inline Source* GetSourceFromRange(SourceRange range) { return GetSourceFromLocation(range.start); }

    private:
        DiagnosticReporter& reporter;
        std::vector<Source> sources{};
    };

}