#include "ghostbyte/config.hpp"
#include "ghostbyte/pressure_analyzer.hpp"
#include "ghostbyte/policy_engine.hpp"
#include "ghostbyte/reclamation_controller.hpp"
#include "ghostbyte/feedback_analyzer.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace ghostbyte;

MemorySnapshot make_snapshot(
    std::uint64_t total,
    std::uint64_t available,
    bool valid = true) {

    MemorySnapshot snapshot;
    snapshot.total_memory_bytes = total;
    snapshot.available_memory_bytes = available;
    snapshot.valid = valid;
    return snapshot;
}

int main() {
    Config config;
    assert(config.is_valid());

    PressureAnalyzer analyzer(config);
    PolicyEngine engine;
    ReclamationController controller;

    const std::uint64_t total = 1000;

    // Default pressure thresholds: 10% Critical, 20% Elevated
    assert(analyzer.analyze(make_snapshot(total, 650)) == PressureLevel::Healthy);
    assert(analyzer.analyze(make_snapshot(total, 200)) == PressureLevel::Elevated);
    assert(analyzer.analyze(make_snapshot(total, 100)) == PressureLevel::Critical);
    assert(analyzer.analyze(make_snapshot(total, 0)) == PressureLevel::Critical);
    assert(analyzer.analyze(make_snapshot(total, 0, false)) == PressureLevel::Unknown);
    assert(analyzer.analyze(make_snapshot(0, 0)) == PressureLevel::Unknown);
    assert(analyzer.analyze(make_snapshot(total, 1100)) == PressureLevel::Unknown);

    // Custom thresholds: 15% Critical, 30% Elevated
    Config custom_config;
    custom_config.critical_threshold_percent = 15.0;
    custom_config.elevated_threshold_percent = 30.0;

    assert(custom_config.is_valid());

    PressureAnalyzer custom_analyzer(custom_config);

    assert(custom_analyzer.analyze(make_snapshot(total, 150)) == PressureLevel::Critical);
    assert(custom_analyzer.analyze(make_snapshot(total, 250)) == PressureLevel::Elevated);
    assert(custom_analyzer.analyze(make_snapshot(total, 400)) == PressureLevel::Healthy);

    // Invalid configuration must be rejected by validation and analysis
    Config invalid_config;
    invalid_config.critical_threshold_percent = 35.0;
    invalid_config.elevated_threshold_percent = 20.0;

    assert(!invalid_config.is_valid());

    PressureAnalyzer invalid_analyzer(invalid_config);
    assert(invalid_analyzer.analyze(make_snapshot(total, 500)) == PressureLevel::Unknown);

    // PolicyEngine tests
    assert(engine.decide(PressureLevel::Healthy).mode == ReclamationMode::None);
    assert(engine.decide(PressureLevel::Elevated).mode == ReclamationMode::Conservative);
    assert(engine.decide(PressureLevel::Critical).mode == ReclamationMode::Aggressive);
    assert(engine.decide(PressureLevel::Unknown).mode == ReclamationMode::None);

    // Workload-aware policy tests
    std::vector<ProcessSnapshot> processes = {
        {101, "small-worker", 10 * 1024, true},
        {202, "large-worker", 50 * 1024, true},
        {303, "invalid-worker", 100 * 1024, false},
        {0, "invalid-pid", 200 * 1024, true}
    };

    PolicyDecision workload_decision =
        engine.decide(PressureLevel::Critical, processes);

    assert(workload_decision.mode == ReclamationMode::Aggressive);
    assert(workload_decision.candidate_pid == 202);
    assert(workload_decision.candidate_name == "large-worker");
    assert(workload_decision.candidate_rss_bytes == 50 * 1024);

    PolicyDecision healthy_workload =
        engine.decide(PressureLevel::Healthy, processes);
    assert(healthy_workload.candidate_pid == -1);

    PolicyDecision empty_workload =
        engine.decide(PressureLevel::Critical, {});
    assert(empty_workload.candidate_pid == -1);
    assert(empty_workload.mode == ReclamationMode::Aggressive);

    // Adaptive policy tests
    FeedbackResult worsening_feedback;
    worsening_feedback.valid = true;
    worsening_feedback.memory_trend = MemoryTrend::Decreased;
    worsening_feedback.pressure_trend = PressureTrend::Worsened;

    PolicyDecision adaptive_worsening = engine.decide(
        PressureLevel::Elevated, processes, worsening_feedback);

    assert(adaptive_worsening.mode == ReclamationMode::Moderate);
    assert(adaptive_worsening.candidate_pid == 202);

    FeedbackResult improving_feedback;
    improving_feedback.valid = true;
    improving_feedback.memory_trend = MemoryTrend::Increased;
    improving_feedback.pressure_trend = PressureTrend::Improved;

    PolicyDecision adaptive_improving = engine.decide(
        PressureLevel::Elevated, processes, improving_feedback);

    assert(adaptive_improving.mode == ReclamationMode::Conservative);

    // Invalid feedback must preserve the baseline policy.
    FeedbackResult invalid_feedback;

    PolicyDecision adaptive_invalid = engine.decide(
        PressureLevel::Elevated, processes, invalid_feedback);

    assert(adaptive_invalid.mode == ReclamationMode::Conservative);

    // Feedback must not override critical or healthy pressure.
    assert(engine.decide(
        PressureLevel::Critical, processes, improving_feedback).mode
        == ReclamationMode::Aggressive);

    assert(engine.decide(
        PressureLevel::Healthy, processes, worsening_feedback).mode
        == ReclamationMode::None);

    // Controller: no reclamation required
    PolicyDecision healthy = engine.decide(PressureLevel::Healthy);
    ReclamationResult result = controller.execute(healthy);

    assert(!result.action_taken);
    assert(result.mode == ReclamationMode::None);
    assert(result.message == "No reclamation required");

    // Controller: conservative dry run
    PolicyDecision elevated = engine.decide(PressureLevel::Elevated);
    result = controller.execute(elevated);

    assert(!result.action_taken);
    assert(result.mode == ReclamationMode::Conservative);
    assert(result.message == "DRY RUN: Conservative reclamation planned");

    // Controller: aggressive dry run
    PolicyDecision critical = engine.decide(PressureLevel::Critical);
    result = controller.execute(critical);

    assert(!result.action_taken);
    assert(result.mode == ReclamationMode::Aggressive);
    assert(result.message == "DRY RUN: Aggressive reclamation planned");

    // Unknown pressure must never trigger an action
    PolicyDecision unknown = engine.decide(PressureLevel::Unknown);
    result = controller.execute(unknown);

    assert(!result.action_taken);
    assert(result.message == "Unknown memory pressure; no action taken");

    // Real reclamation is not implemented yet
    result = controller.execute(critical, false);

    assert(!result.action_taken);
    assert(result.message == "Real reclamation is not implemented yet");

        // FeedbackAnalyzer tests
    FeedbackAnalyzer feedback_analyzer;

    MemorySnapshot before;
    before.total_memory_bytes = 1000;
    before.available_memory_bytes = 100;
    before.valid = true;

    MemorySnapshot after;
    after.total_memory_bytes = 1000;
    after.available_memory_bytes = 200;
    after.valid = true;

    // Available memory increased; pressure improved
    FeedbackResult feedback = feedback_analyzer.analyze(
        before, after,
        PressureLevel::Critical,
        PressureLevel::Elevated
    );

    assert(feedback.valid);
    assert(feedback.available_memory_delta_bytes == 100);
    assert(feedback.memory_trend == MemoryTrend::Increased);
    assert(feedback.pressure_trend == PressureTrend::Improved);

    // Available memory decreased; pressure worsened
    after.available_memory_bytes = 50;

    feedback = feedback_analyzer.analyze(
        before, after,
        PressureLevel::Healthy,
        PressureLevel::Critical
    );

    assert(feedback.valid);
    assert(feedback.available_memory_delta_bytes == -50);
    assert(feedback.memory_trend == MemoryTrend::Decreased);
    assert(feedback.pressure_trend == PressureTrend::Worsened);

    // Available memory and pressure stayed unchanged
    after.available_memory_bytes = before.available_memory_bytes;

    feedback = feedback_analyzer.analyze(
        before, after,
        PressureLevel::Elevated,
        PressureLevel::Elevated
    );

    assert(feedback.valid);
    assert(feedback.available_memory_delta_bytes == 0);
    assert(feedback.memory_trend == MemoryTrend::Unchanged);
    assert(feedback.pressure_trend == PressureTrend::Unchanged);

    // Invalid snapshot must produce invalid feedback
    after.valid = false;

    feedback = feedback_analyzer.analyze(
        before, after,
        PressureLevel::Healthy,
        PressureLevel::Critical
    );

    assert(!feedback.valid);
    assert(feedback.memory_trend == MemoryTrend::Unknown);
    assert(feedback.pressure_trend == PressureTrend::Unknown);

    // Unknown pressure must produce invalid feedback
    after.valid = true;

    feedback = feedback_analyzer.analyze(
        before, after,
        PressureLevel::Unknown,
        PressureLevel::Healthy
    );

    assert(!feedback.valid);

    std::cout << "All GhostBytes tests passed!\n";

    return 0;
}
