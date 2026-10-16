#include "ghostbyte/pressure_analyzer.hpp"

namespace ghostbyte {

PressureLevel PressureAnalyzer::analyze(
    const MemorySnapshot& snapshot) const {

    if (!snapshot.valid ||
        snapshot.total_memory_bytes == 0 ||
        snapshot.available_memory_bytes > snapshot.total_memory_bytes) {
        return PressureLevel::Unknown;
    }

    const double available_percent =
        (static_cast<double>(snapshot.available_memory_bytes) /
         static_cast<double>(snapshot.total_memory_bytes)) * 100.0;

    if (available_percent <= 10.0) {
        return PressureLevel::Critical;
    }

    if (available_percent <= 20.0) {
        return PressureLevel::Elevated;
    }

    return PressureLevel::Healthy;
}

}
