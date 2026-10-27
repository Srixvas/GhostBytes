#include "ghostbyte/config.hpp"
#include "ghostbyte/pressure_analyzer.hpp"
#include "ghostbyte/policy_engine.hpp"
#include "ghostbyte/reclamation_controller.hpp"

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

    std::cout << "All GhostBytes tests passed!\n";

    return 0;
}
