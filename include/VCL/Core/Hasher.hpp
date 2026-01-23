#pragma once

#include <llvm/ADT/StringRef.h>

#include <cstdint>


namespace VCL {

    class Hasher {
    public:
        inline void Hash(uint64_t v) { value ^= v + 0x9e3779b97f4a7c15 + (value << 6) + (value >> 2); }
        inline void Hash(llvm::StringRef string) {
            // FNV-1a
            uint64_t hash = 0xCBF29CE484222325;
            for (const char c : string)
                hash = (hash ^ (uint64_t)c) * 0x100000001B3;
            Hash(hash);
        }

        inline uint64_t Get() const { return value; }

    private:
        uint64_t value = 0;
    };

}