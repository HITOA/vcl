#pragma once

#include <string>
#include <exception>
#include <cstdint>
#include <expected>


namespace VCL {

    /**
     * @brief represents a simple error struct.
     */
    struct Error {
        std::string message;
    };

    class Exception : public std::runtime_error {
    public:
        explicit Exception(const std::string& msg) :
            std::runtime_error{ msg }, position{ 0 }, line{ 0 } {};
        explicit Exception(const std::string& msg, uint32_t position, uint32_t line) :
            std::runtime_error{ msg }, position{ position }, line{ line } {};
        Exception(const Exception&) = default;
        Exception& operator=(const Exception&) = default;
        Exception(Exception&&) = default;
        Exception& operator=(Exception&&) = default;

    public:
        uint32_t position;
        uint32_t line;
    };

    template<typename T>
    inline T ThrowOnError(std::expected<T, Error> value, uint32_t position, uint32_t line) {
        if (value.has_value())
            return *value;
        throw Exception{ value.error().message, position, line };
    }
}