#pragma once

#include <VCL/Core/Handle.hpp>
#include <VCL/Core/SourceError.hpp>
#include <VCL/Core/SourceLocation.hpp>

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/MemoryBuffer.h>

#include <expected>
#include <cstdint>
#include <vector>


namespace VCL {
    using SourceID = uint32_t;

    /**
     * Store a buffer representing a VCL Source as a llvm::MemoryBuffer
     * It provide the interface to work with said buffer and Create one from disk or memory.
     * a Source will usually be created from a file on disk, but for VCL it can also
     * be created from memory. In that case it can cause issue with debugging symbol not
     * being attached to a proper file on the disk.
     */
    class Source {
    public:
        Source() = default;
        Source(const Source& source) = default;
        Source(Source&& source) = default;
        ~Source() = default;

        inline Source& operator=(const Source& source) = default;
        inline Source& operator=(Source&& source) = default;

        /** Get a reference to the stored MemoryBuffer containing the source */
        inline llvm::MemoryBufferRef GetBufferRef() const {
            return buffer->getMemBufferRef();
        }

        /**
         * Return the identifier of this Source stored in the MemoryBuffer.
         * If this Source was loaded from disk, the identifier will be its filename.
         * If this Source was loaded from memory, the identifier will be the name
         * given to the LoadFromMemory function. or an empty string if none was given.
         */
        inline llvm::StringRef GetBufferIdentifier() const {
            return buffer->getBufferIdentifier();
        }
        
        std::pair<uint32_t, uint32_t> GetLineColumn(uint32_t offset) const;

        /** 
         * Load a Source from disk.
         * If this file doesn't exist or cannot be opened, a SourceError with the corresponding
         * error will be returned instead.
         */
        static std::expected<Handle<Source>, SourceError> LoadFromDisk(llvm::StringRef path);
        /**
         * Load a Source from memory from the given buffer.
         * a name can also be given and will be the Source's buffer identifier.
         */
        static std::expected<Handle<Source>, SourceError> LoadFromMemory(llvm::StringRef buffer, llvm::StringRef name = "");

    private:
        void CalculateLineOffsets();
        
    private:
        std::unique_ptr<llvm::MemoryBuffer> buffer{};
        std::vector<uint32_t> lineOffsets{};
    };

}