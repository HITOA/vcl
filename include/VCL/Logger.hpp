#pragma once

#include <VCL/Source.hpp>

#include <string>
#include <format>


namespace VCL {

    struct Message {
        enum class MessageSeverity {
            None = 0,
            Error = 1,
            Warning = 2,
            Info = 3,
            Debug = 4
        } severity = MessageSeverity::None;
        std::string message;
    };
    
    class Logger {
    public:
        virtual ~Logger() = default;

        virtual void Log(Message& message) {};

        template<typename... Args>
        inline void Error(std::format_string<Args...> format, Args&&... args) {
            Message message{
                Message::MessageSeverity::Error,
                std::vformat(format.get(), std::make_format_args(args...))
            };
            Log(message);
        }

        template<typename... Args>
        inline void Warning(std::format_string<Args...> format, Args&&... args) {
            Message message{
                Message::MessageSeverity::Warning,
                std::vformat(format.get(), std::make_format_args(args...))
            };
            Log(message);
        }

        template<typename... Args>
        inline void Info(std::format_string<Args...> format, Args&&... args) {
            Message message{
                Message::MessageSeverity::Info,
                std::vformat(format.get(), std::make_format_args(args...))
            };
            Log(message);
        }

        template<typename... Args>
        inline void Debug(std::format_string<Args...> format, Args&&... args) {
            Message message{
                Message::MessageSeverity::Debug,
                std::vformat(format.get(), std::make_format_args(args...))
            };
            Log(message);
        }
    };

}