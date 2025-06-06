#pragma once

#include <VCL/Source.hpp>

#include <string>
#include <exception>
#include <cstdint>
#include <expected>
#include <stdexcept>


namespace VCL {
    using Error = std::string;

    class Exception : public std::runtime_error {
    public:
        Exception(const std::string& msg) = delete;
        explicit Exception(const std::string& msg, SourceLocation location) :
            std::runtime_error{ msg }, location{ location } {};
        Exception(const Exception&) = default;
        Exception& operator=(const Exception&) = default;
        Exception(Exception&&) = default;
        Exception& operator=(Exception&&) = default;
        
    public:
        SourceLocation location;
    };
    
    template<typename T>
    inline T ThrowOnError(std::expected<T, Error> value, SourceLocation location) {
        if (value.has_value())
            return *value;
        throw Exception{ value.error(), location };
    }
}