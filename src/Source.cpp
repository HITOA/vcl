#include <VCL/Source.hpp>

#include <fstream>
#include <format>

#include <iostream>


std::expected<std::shared_ptr<VCL::Source>, std::string> VCL::Source::LoadFromDisk(std::filesystem::path& path) {
    std::ifstream file{ path, std::ios::binary };

    if (!file.is_open())
        return std::unexpected(std::format("Unable to open source file \'{}\' from disk.", path.filename().c_str()));

    std::stringstream buffer{};
    buffer << file.rdbuf();
    std::string source = buffer.str();
    file.close();

    return std::make_shared<VCL::Source>(source, path);
}

std::string GetLineFromString(std::string str, uint32_t line) {
    uint32_t last_line_offset = 0;
    uint32_t current_line = 0;

    for (uint32_t offset = 0; offset < str.length(); ++offset) {
        if (str[offset] == '\n') {
            if (current_line == line) {
                return str.substr(last_line_offset, offset - last_line_offset);
            } else {
                last_line_offset = offset + 1;
                ++current_line;
            }
        }
    }

    return str.substr(last_line_offset, str.length() - last_line_offset);
}

std::string VCL::SourceLocation::ToString() {
    if (source == nullptr)
        return "NULL";
    return std::format("{}:{}:{}", std::filesystem::weakly_canonical(source->path).string(), line + 1, position);
}

std::string VCL::SourceLocation::ToStringDetailed() {
    if (source == nullptr)
        return "NULL";
    std::string error_line = GetLineFromString(source->source, line);
    std::string pointer_message;
    pointer_message.resize(error_line.length());
    for (uint32_t i = 0; i < error_line.length(); ++i) {
        if (i < position)
            pointer_message[i] = ' ';
        else if (i == position)
            pointer_message[i] = '^';
        else if (i - position < length)
            pointer_message[i] = '~';
        else
            pointer_message[i] = ' ';
    }
    std::string padding = std::to_string(line + 1);
    for (uint32_t i = 0; i < padding.length(); ++i)
        padding[i] = ' ';
    return std::format("\t{} | {}\n\t{} | {}", line + 1, error_line, padding, pointer_message);
}