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

}