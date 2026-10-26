
#include "ghostbyte/pressure_analyzer.hpp"
#include "ghostbyte/policy_engine.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>

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
    PressureAnalyzer analyzer;
    PolicyEngine engine;

    const std::uint64_t total = 1000;

    // PressureAnalyzer tests
    assert(analyzer.analyze(make_snapshot(total, 650)) == PressureLevel::Healthy);
    assert(analyzer.analyze(make_snapshot(total, 200)) == PressureLevel::Elevated);
    assert(analyzer.analyze(make_snapshot(total, 100)) == PressureLevel::Critical);
    assert(analyzer.analyze(make_snapshot(total, 0)) == PressureLevel::Critical);

    assert(analyzer.analyze(make_snapshot(total, 0, false)) == PressureLevel::Unknown);
    assert(analyzer.analyze(make_snapshot(0, 0)) == PressureLevel::Unknown);
    assert(analyzer.analyze(make_snapshot(total, 1100)) == PressureLevel::Unknown);

    // PolicyEngine tests
    assert(engine.decide(PressureLevel::Healthy).mode == ReclamationMode::None);
    assert(engine.decide(PressureLevel::Elevated).mode == ReclamationMode::Conservative);
    assert(engine.decide(PressureLevel::Critical).mode == ReclamationMode::Aggressive);
    assert(engine.decide(PressureLevel::Unknown).mode == ReclamationMode::None);

    std::cout << "All PressureAnalyzer and PolicyEngine tests passed!\n";

    return 0;
}
