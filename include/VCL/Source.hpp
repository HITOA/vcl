#pragma once

#include <string>
#include <cstdint>
#include <filesystem>
#include <expected>


namespace VCL {
    struct SourceLocation;
    
    struct Source {
        std::string source;
        std::filesystem::path path;

        static std::expected<std::shared_ptr<Source>, std::string> LoadFromDisk(std::filesystem::path& path);
    };

    struct SourceLocation {
        std::shared_ptr<Source> source;
        uint32_t position;
        uint32_t line;
        uint32_t length;

        std::string ToString();
        std::string ToStringDetailed();
    };
}