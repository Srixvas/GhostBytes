#include "ghostbyte/policy_engine.hpp"
#include <algorithm>

namespace ghostbyte {
namespace {
    constexpr std::uint64_t MAX_RECLAIM_BYTES = 64ULL * 1024 * 1024; // 64 MiB safety cap

    void set_dynamic_target(PolicyDecision& decision) {
        if (decision.mode == ReclamationMode::None || decision.candidate_rss_bytes == 0) {
            decision.target_reclaim_bytes = 0;
            return;
        }
        std::uint64_t target = 0;
        switch (decision.mode) {
            case ReclamationMode::Conservative:
                target = decision.candidate_rss_bytes / 10; // 10% of workload
                break;
            case ReclamationMode::Moderate:
                target = decision.candidate_rss_bytes / 5;  // 20% of workload
                break;
            case ReclamationMode::Aggressive:
                target = decision.candidate_rss_bytes / 3;  // ~33% of workload
                break;
            default:
                break;
        }
        decision.target_reclaim_bytes = std::min(target, MAX_RECLAIM_BYTES);
    }
}

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
            decision.reason = "Memory pressure is unknown; no action recommended";
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
        decision.candidate_rss_bytes = largest->resident_memory_bytes;

        decision.reason = pressure == PressureLevel::Critical
            ? "Critical memory pressure; largest RSS process identified for assessment"
            : "Elevated memory pressure; largest RSS process identified for assessment";
    }

    set_dynamic_target(decision);
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

    set_dynamic_target(decision);
    return decision;
}

PolicyDecision PolicyEngine::decide(
    PressureLevel pressure,
    const std::vector<ProcessSnapshot>& processes,
    const FeedbackResult& feedback,
    const ReclamationFeedback& reclamation_feedback) const {

    PolicyDecision decision = decide(pressure, processes, feedback);

    if (!reclamation_feedback.valid ||
        pressure != PressureLevel::Elevated) {
        return decision;
    }

    if (reclamation_feedback.observed_reclaimed_bytes == 0) {
        if (decision.mode == ReclamationMode::Conservative) {
            decision.mode = ReclamationMode::Moderate;
            decision.reason =
                "Elevated pressure persists and no memory decrease was observed after reclamation";
        }
    } else if (decision.mode == ReclamationMode::Conservative) {
        decision.reason =
            "Previous reclamation coincided with an observed memory decrease";
    }

    set_dynamic_target(decision);
    return decision;
}

}
