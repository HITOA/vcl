#pragma once

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
        Source() = delete;
        Source(std::unique_ptr<llvm::MemoryBuffer> buffer) : buffer{ std::move(buffer) } {}
        Source(const Source& other) = delete;
        Source(Source&& other) = default;
        ~Source() = default;

        inline Source& operator=(const Source& other) = delete;
        inline Source& operator=(Source&& other) = default;

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
        
    private:
        std::unique_ptr<llvm::MemoryBuffer> buffer{};
    };

}