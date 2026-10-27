#include "ghostbyte/system_monitor.hpp"

#include <cstdint>
#include <fstream>
#include <filesystem>
#include <system_error>
#include <vector>
#include <sstream>
#include <string>
#include <unordered_map>

namespace ghostbyte {

MemorySnapshot SystemMonitor::capture() const {
    MemorySnapshot snapshot;
    std::ifstream file("/proc/meminfo");

    if (!file.is_open()) {
        return snapshot;
    }

    std::unordered_map<std::string, std::uint64_t> memory_info;
    std::string line;

    while (std::getline(file, line)) {
        std::istringstream stream(line);
        std::string key;
        std::uint64_t value;

        if (!(stream >> key >> value)) {
            continue;
        }

        std::string unit;
        stream >> unit;

        if (unit == "kB") {
            value *= 1024;
        }

        memory_info[key] = value;
    }

    if (!memory_info.count("MemTotal:") ||
        !memory_info.count("MemAvailable:") ||
        !memory_info.count("SwapTotal:") ||
        !memory_info.count("SwapFree:")) {
        return snapshot;
    }

    snapshot.total_memory_bytes = memory_info["MemTotal:"];
    snapshot.available_memory_bytes = memory_info["MemAvailable:"];
    snapshot.swap_total_bytes = memory_info["SwapTotal:"];
    snapshot.swap_free_bytes = memory_info["SwapFree:"];
    snapshot.valid = snapshot.total_memory_bytes > 0 &&
                     snapshot.available_memory_bytes <=
                         snapshot.total_memory_bytes;

    return snapshot;
}


std::vector<ProcessSnapshot> SystemMonitor::capture_processes() const {
    std::vector<ProcessSnapshot> processes;
    std::error_code ec;
    std::filesystem::directory_iterator it("/proc", ec);
    const std::filesystem::directory_iterator end;

    while (!ec && it != end) {
        const auto entry = *it;
        it.increment(ec);

        const std::string directory_name =
            entry.path().filename().string();

        if (directory_name.empty() ||
            directory_name.find_first_not_of("0123456789") !=
                std::string::npos) {
            continue;
        }

        ProcessSnapshot process;

        try {
            process.pid = std::stoi(directory_name);
        } catch (const std::exception&) {
            continue;
        }

        std::ifstream status_file(entry.path() / "status");
        if (!status_file) {
            continue;
        }

        bool found_name = false;
        bool found_rss = false;
        std::string line;

        while (std::getline(status_file, line)) {
            if (line.rfind("Name:", 0) == 0) {
                std::istringstream stream(line.substr(5));
                stream >> process.name;
                found_name = !process.name.empty();
            } else if (line.rfind("VmRSS:", 0) == 0) {
                std::istringstream stream(line.substr(6));
                std::uint64_t rss_kb = 0;
                std::string unit;

                if (stream >> rss_kb >> unit && unit == "kB") {
                    process.resident_memory_bytes = rss_kb * 1024;
                    found_rss = true;
                }
            }
        }

        process.valid = found_name && found_rss;
        if (process.valid) {
            processes.push_back(process);
        }
    }

    return processes;
}

}