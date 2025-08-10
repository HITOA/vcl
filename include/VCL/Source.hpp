#pragma once

#include <string>
#include <cstdint>
#include <filesystem>
#include <expected>
#include <string_view>


namespace VCL {
    struct SourceLocation;
    
    struct Source {
        std::string source;
        std::filesystem::path path;
        
        static std::expected<std::shared_ptr<Source>, std::string> LoadFromDisk(const std::filesystem::path& path);
        static std::expected<std::shared_ptr<Source>, std::string> LoadFromDisk(std::filesystem::path&& path);
        static std::expected<std::shared_ptr<Source>, std::string> LoadFromMemory(const std::string& buffer);
    };

    struct SourceLocation {
        std::shared_ptr<Source> source;
        uint32_t position;
        uint32_t line;
        uint32_t length;

        std::string ToString() const;
        std::string ToStringDetailed() const;
        SourceLocation Combine(SourceLocation location);
    };
}