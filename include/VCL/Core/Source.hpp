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

    class Source {
    public:
        Source() = default;
        Source(const Source& source) = default;
        Source(Source&& source) = default;
        ~Source() = default;

        inline Source& operator=(const Source& source) = default;
        inline Source& operator=(Source&& source) = default;

        inline llvm::MemoryBufferRef GetBufferRef() const {
            return buffer->getMemBufferRef();
        }

        inline llvm::StringRef GetBufferIdentifier() const {
            return buffer->getBufferIdentifier();
        }
        
        std::pair<uint32_t, uint32_t> GetLineColumn(uint32_t offset) const;

        static std::expected<Handle<Source>, SourceError> LoadFromDisk(llvm::StringRef path);
        static std::expected<Handle<Source>, SourceError> LoadFromMemory(llvm::StringRef buffer, llvm::StringRef name = "");

    private:
        void CalculateLineOffsets();
        
    private:
        std::unique_ptr<llvm::MemoryBuffer> buffer{};
        std::vector<uint32_t> lineOffsets{};
    };

}