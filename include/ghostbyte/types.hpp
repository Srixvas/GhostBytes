
#ifndef GHOSTBYTE_TYPES_HPP
#define GHOSTBYTE_TYPES_HPP

#include <cstdint>
#include <string>

namespace ghostbyte {

enum class PressureLevel {
    Healthy,
    Elevated,
    Critical,
    Unknown
};

enum class ReclamationMode {
    None,
    Conservative,
    Moderate,
    Aggressive
};

struct MemorySnapshot {
    std::uint64_t total_memory_bytes = 0;
    std::uint64_t available_memory_bytes = 0;
    std::uint64_t swap_total_bytes = 0;
    std::uint64_t swap_free_bytes = 0;
    bool valid = false;
};

struct ProcessSnapshot {
    int pid = 0;
    std::string name;
    std::uint64_t resident_memory_bytes = 0;
    bool valid = false;
};

struct PolicyDecision {
    PressureLevel pressure = PressureLevel::Unknown;
    ReclamationMode mode = ReclamationMode::None;
    const char* reason = "No decision made";
    int candidate_pid = -1;
    std::string candidate_name;
    std::uint64_t candidate_rss_bytes = 0;
};

}

#endif
