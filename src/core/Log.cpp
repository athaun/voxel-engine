#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>

#include "Log.h"

namespace Log {
    void log(std::string level, const std::string& message) {
        std::ostringstream oss;
        oss << "[" << level << "] " << message;
        std::cout << oss.str() << std::endl;
    }

    void debug(const std::string& message) {
        log("DEBUG", message);
    }

    void info(const std::string& message) {
        log("INFO", message);
    }

    void warn(const std::string& message) {
        log("WARNING", message);
    }

    void error(const std::string& message) {
        log("ERROR", message);
    }
}