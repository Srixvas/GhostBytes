#include "ghostbyte/feedback_analyzer.hpp"

namespace ghostbyte {

FeedbackResult FeedbackAnalyzer::analyze(
    const MemorySnapshot& before,
    const MemorySnapshot& after,
    PressureLevel pressure_before,
    PressureLevel pressure_after) const {

    FeedbackResult result;

    if (!before.valid || !after.valid ||
        before.total_memory_bytes == 0 ||
        after.total_memory_bytes == 0 ||
        before.available_memory_bytes > before.total_memory_bytes ||
        after.available_memory_bytes > after.total_memory_bytes ||
        pressure_before == PressureLevel::Unknown ||
        pressure_after == PressureLevel::Unknown) {
        return result;
    }

    const std::uint64_t before_available =
        before.available_memory_bytes;
    const std::uint64_t after_available =
        after.available_memory_bytes;

    if (after_available >= before_available) {
        const std::uint64_t delta = after_available - before_available;

        if (delta > static_cast<std::uint64_t>(
                        INT64_MAX)) {
            return result;
        }

        result.available_memory_delta_bytes =
            static_cast<std::int64_t>(delta);
    } else {
        const std::uint64_t delta = before_available - after_available;

        if (delta > static_cast<std::uint64_t>(
                        INT64_MAX)) {
            return result;
        }

        result.available_memory_delta_bytes =
            -static_cast<std::int64_t>(delta);
    }

    if (result.available_memory_delta_bytes > 0) {
        result.memory_trend = MemoryTrend::Increased;
    } else if (result.available_memory_delta_bytes < 0) {
        result.memory_trend = MemoryTrend::Decreased;
    } else {
        result.memory_trend = MemoryTrend::Unchanged;
    }

    const auto pressure_rank = [](PressureLevel pressure) {
        switch (pressure) {
            case PressureLevel::Healthy:  return 0;
            case PressureLevel::Elevated: return 1;
            case PressureLevel::Critical: return 2;
            case PressureLevel::Unknown:  return -1;
        }
        return -1;
    };

    const int before_rank = pressure_rank(pressure_before);
    const int after_rank = pressure_rank(pressure_after);

    if (after_rank < before_rank) {
        result.pressure_trend = PressureTrend::Improved;
    } else if (after_rank > before_rank) {
        result.pressure_trend = PressureTrend::Worsened;
    } else {
        result.pressure_trend = PressureTrend::Unchanged;
    }

    result.valid = true;
    return result;
}

}
