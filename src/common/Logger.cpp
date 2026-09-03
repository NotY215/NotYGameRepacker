#include "noty/common/Logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace noty {

    Logger& Logger::instance() {
        static Logger instance;
        return instance;
    }

    void Logger::log(const std::string& message, LogLevel level) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "[%H:%M:%S");
        ss << "." << std::setfill('0') << std::setw(3) << ms.count() << "] ";

        switch (level) {
        case LogLevel::Info:    ss << "[INFO] "; break;
        case LogLevel::Warning: ss << "[WARN] "; break;
        case LogLevel::Error:   ss << "[ERROR] "; break;
        case LogLevel::Debug:   ss << "[DEBUG] "; break;
        }

        ss << message << std::endl;
        std::cout << ss.str();
    }

} // namespace noty