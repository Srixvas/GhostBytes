#include "ghostbyte/system_monitor.hpp"

#include <cstdint>
#include <fstream>
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

}