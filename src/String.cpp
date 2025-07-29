#include "String.hpp"

#include <utility>

std::string VCL::ParseRawString(const std::string& str) {
    std::pair<char, char> escapeSequence[] = {
        { '\\', '\\' },
        { 'n', '\n' },
        { 'r', '\r' },
        { 't', '\t' },
        { '\"', '\"' }
    };

    std::string parsedStr{};
    parsedStr.reserve(str.size());

    for (size_t i = 1; i < str.size() - 1; ++i) {
        if (str[i] == '\\' && i + 1 < str.size() - 1) {
            ++i;
            for (size_t i = 0; i < 5; ++i) {
                if (escapeSequence[i].first == str[i]) {
                    parsedStr += escapeSequence[i].second;
                    break;
                }
            }
            continue;
        }

        parsedStr += str[i];
    }

    return parsedStr;
}