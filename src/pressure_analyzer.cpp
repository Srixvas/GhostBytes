#include "ghostbyte/pressure_analyzer.hpp"

namespace ghostbyte {

PressureAnalyzer::PressureAnalyzer(const Config& config)
    : config_(config) {}

PressureLevel PressureAnalyzer::analyze(
    const MemorySnapshot& snapshot) const {

    if (!config_.is_valid() ||
        !snapshot.valid ||
        snapshot.total_memory_bytes == 0 ||
        snapshot.available_memory_bytes > snapshot.total_memory_bytes) {
        return PressureLevel::Unknown;
    }

    const double available_percent =
        (static_cast<double>(snapshot.available_memory_bytes) /
         static_cast<double>(snapshot.total_memory_bytes)) * 100.0;

    if (available_percent <= config_.critical_threshold_percent) {
        return PressureLevel::Critical;
    }

    if (available_percent <= config_.elevated_threshold_percent) {
        return PressureLevel::Elevated;
    }

    return PressureLevel::Healthy;
}

}
