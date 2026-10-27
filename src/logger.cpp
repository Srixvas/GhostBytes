#include "ghostbyte/logger.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace ghostbyte {

Logger::Logger(const std::string& file_path)
    : file_path_(file_path) {}

bool Logger::log(const std::string& message) const {
    const std::filesystem::path path(file_path_);

    if (path.has_parent_path()) {
        std::error_code error;
        std::filesystem::create_directories(path.parent_path(), error);

        if (error) {
            return false;
        }
    }

    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);

    std::tm local_time{};

    if (localtime_r(&time, &local_time) == nullptr) {
        return false;
    }

    std::ostringstream timestamp;
    timestamp << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");

    std::ofstream file(file_path_, std::ios::app);

    if (!file.is_open()) {
        return false;
    }

    file << '[' << timestamp.str() << "] " << message << '\n';

    return file.good();
}

}
