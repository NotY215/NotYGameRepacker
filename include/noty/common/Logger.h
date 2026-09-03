#pragma once
#include <string>
#include <mutex>

namespace noty {

    enum class LogLevel {
        Info,
        Warning,
        Error,
        Debug
    };

    class Logger {
    public:
        static Logger& instance();

        void log(const std::string& message, LogLevel level = LogLevel::Info);

        inline void info(const std::string& msg) { log(msg, LogLevel::Info); }
        inline void warning(const std::string& msg) { log(msg, LogLevel::Warning); }
        inline void error(const std::string& msg) { log(msg, LogLevel::Error); }
        inline void debug(const std::string& msg) { log(msg, LogLevel::Debug); }

    private:
        Logger() = default;
        ~Logger() = default;
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        std::mutex m_mutex;
    };

} // namespace noty