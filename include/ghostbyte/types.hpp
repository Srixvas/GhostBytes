
#ifndef GHOSTBYTE_TYPES_HPP
#define GHOSTBYTE_TYPES_HPP

#include <cstdint>
#include <string>

namespace ghostbyte {

enum class MemoryTrend {
    Increased,
    Decreased,
    Unchanged,
    Unknown
};

enum class PressureTrend {
    Improved,
    Worsened,
    Unchanged,
    Unknown
};

struct FeedbackResult {
    std::int64_t available_memory_delta_bytes = 0;
    MemoryTrend memory_trend = MemoryTrend::Unknown;
    PressureTrend pressure_trend = PressureTrend::Unknown;
    bool valid = false;
};

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

struct ReclamationFeedback {
    bool valid = false;
    std::uint64_t observed_reclaimed_bytes = 0;
};

struct PolicyDecision {
    PressureLevel pressure = PressureLevel::Unknown;
    ReclamationMode mode = ReclamationMode::None;
    const char* reason = "No decision made";
    int candidate_pid = -1;
    std::string candidate_name;
    std::uint64_t candidate_rss_bytes = 0;
    std::uint64_t target_reclaim_bytes = 0;
};

}

#endif
