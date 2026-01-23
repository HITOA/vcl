#pragma once

#include <string>
#include <cstddef>


namespace VCL {

    /**
     * Format a given string with the given array of string arguments.
     * Format string use %n to format a given argument in place. %% to escape the percent symbol.
     */
    inline std::string FormatString(const std::string& format, std::string* args, size_t count) {
        std::string output{};
        output.reserve(format.size()); // At least the size of format.size()
        for (size_t i = 0; i < format.size(); ++i) {
            if (format[i] == '%') {
                ++i;
                if (format[i] == '%') {
                    output += "%";
                    continue;
                }
                std::string argIndexStr{ format[i] };
                int currentArgIndex = std::stoi(argIndexStr);
                if (currentArgIndex >= count) {
                    output += '%';
                    output += argIndexStr;
                    continue;
                }
                output += args[currentArgIndex];
                continue;
            }
            output += format[i];
        }
        return output;
    }

    inline std::string ParseStringLiteral(const std::string& literal) {
        if (literal.size() < 2 || literal.front() != '"' || literal.back() != '"')
            return literal;
        
        std::string result{};
        result.reserve(literal.size());

        for (int i = 1; i < literal.size() - 1; ++i) {
            if (literal[i] == '\\' && i + 1 < literal.size() - 1) {
                switch (literal[++i]) {
                    case 'n':  result += '\n'; break;
                    case 't':  result += '\t'; break;
                    case 'r':  result += '\r'; break;
                    case '\\': result += '\\'; break;
                    case '"':  result += '"';  break;
                    case '0':  result += '\0'; break;
                    case 'a':  result += '\a'; break;
                    case 'b':  result += '\b'; break;
                    case 'f':  result += '\f'; break;
                    case 'v':  result += '\v'; break;
                    default:   result += literal[i]; break;
                }
            } else {
                result += literal[i];
            }
        }

        return result;
    }

}