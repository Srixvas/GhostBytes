#include "ghostbyte/policy_engine.hpp"

namespace ghostbyte {

PolicyDecision PolicyEngine::decide(PressureLevel pressure) const {
    PolicyDecision decision;
    decision.pressure = pressure;

    switch (pressure) {
        case PressureLevel::Healthy:
            decision.mode = ReclamationMode::None;
            decision.reason = "Memory availability is healthy";
            break;

        case PressureLevel::Elevated:
            decision.mode = ReclamationMode::Conservative;
            decision.reason = "Memory pressure is elevated";
            break;

        case PressureLevel::Critical:
            decision.mode = ReclamationMode::Aggressive;
            decision.reason = "Memory pressure is critical";
            break;

        case PressureLevel::Unknown:
        default:
            decision.mode = ReclamationMode::None;
            decision.reason =
                "Memory pressure is unknown; no action recommended";
            break;
    }

    return decision;
}

PolicyDecision PolicyEngine::decide(
    PressureLevel pressure,
    const std::vector<ProcessSnapshot>& processes) const {

    PolicyDecision decision = decide(pressure);

    if (pressure == PressureLevel::Unknown ||
        pressure == PressureLevel::Healthy) {
        return decision;
    }

    const ProcessSnapshot* largest = nullptr;

    for (const auto& process : processes) {
        if (!process.valid ||
            process.pid <= 0 ||
            process.resident_memory_bytes == 0) {
            continue;
        }

        if (largest == nullptr ||
            process.resident_memory_bytes >
                largest->resident_memory_bytes) {
            largest = &process;
        }
    }

    if (largest != nullptr) {
        decision.candidate_pid = largest->pid;
        decision.candidate_name = largest->name;
        decision.candidate_rss_bytes =
            largest->resident_memory_bytes;

        decision.reason = pressure == PressureLevel::Critical
            ? "Critical memory pressure; largest RSS process identified for assessment"
            : "Elevated memory pressure; largest RSS process identified for assessment";
    }

    return decision;
}

PolicyDecision PolicyEngine::decide(
    PressureLevel pressure,
    const std::vector<ProcessSnapshot>& processes,
    const FeedbackResult& feedback) const {

    PolicyDecision decision = decide(pressure, processes);

    // Current pressure takes priority over historical trends.
    if (pressure != PressureLevel::Elevated || !feedback.valid) {
        return decision;
    }

    const bool pressure_worsened =
        feedback.pressure_trend == PressureTrend::Worsened;

    const bool memory_falling_without_pressure_improvement =
        feedback.pressure_trend == PressureTrend::Unchanged &&
        feedback.memory_trend == MemoryTrend::Decreased;

    if (pressure_worsened ||
        memory_falling_without_pressure_improvement) {
        decision.mode = ReclamationMode::Moderate;
        decision.reason =
            "Elevated pressure is worsening or available memory is falling; moderate response recommended";
    }

    return decision;
}

}
