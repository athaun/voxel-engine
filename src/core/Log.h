#pragma once
#include <string>

namespace Log {
    enum class Level {
        Debug,
        Info,
        Warn,
        Error
    };

    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);
}