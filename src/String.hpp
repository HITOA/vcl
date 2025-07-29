#pragma once

#include <string>


namespace VCL {
    /**
     * @brief Take a raw string of the form "string" (quote included) and return the parsed string correctly escaped without quote.
     */
    std::string ParseRawString(const std::string& str);
}