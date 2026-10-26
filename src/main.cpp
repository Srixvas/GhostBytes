#include "ghostbyte/system_monitor.hpp"
#include "ghostbyte/pressure_analyzer.hpp"
#include "ghostbyte/policy_engine.hpp"
#include "ghostbyte/reclamation_controller.hpp"

#include <iomanip>
#include <iostream>

namespace {

const char* pressure_name(ghostbyte::PressureLevel pressure) {
    using ghostbyte::PressureLevel;

    switch (pressure) {
        case PressureLevel::Healthy:  return "Healthy";
        case PressureLevel::Elevated: return "Elevated";
        case PressureLevel::Critical: return "Critical";
        case PressureLevel::Unknown:  return "Unknown";
    }

    return "Unknown";
}

const char* mode_name(ghostbyte::ReclamationMode mode) {
    using ghostbyte::ReclamationMode;

    switch (mode) {
        case ReclamationMode::None:         return "None";
        case ReclamationMode::Conservative: return "Conservative";
        case ReclamationMode::Moderate:     return "Moderate";
        case ReclamationMode::Aggressive:  return "Aggressive";
    }

    return "Unknown";
}

}

int main() {
    using namespace ghostbyte;

    SystemMonitor monitor;
    PressureAnalyzer analyzer;
    PolicyEngine engine;
    ReclamationController controller;

    MemorySnapshot snapshot = monitor.capture();

    if (!snapshot.valid) {
        std::cerr << "Failed to collect valid memory statistics.\n";
        return 1;
    }

    const double total_mib =
        static_cast<double>(snapshot.total_memory_bytes) / (1024.0 * 1024.0);

    const double available_mib =
        static_cast<double>(snapshot.available_memory_bytes) / (1024.0 * 1024.0);

    const double available_percent =
        snapshot.total_memory_bytes == 0
            ? 0.0
            : static_cast<double>(snapshot.available_memory_bytes) /
                  static_cast<double>(snapshot.total_memory_bytes) * 100.0;

    const PressureLevel pressure = analyzer.analyze(snapshot);
    const PolicyDecision decision = engine.decide(pressure);
    const ReclamationResult result = controller.execute(decision, true);

    std::cout << "========== GhostBytes ==========\n";
    std::cout << std::fixed << std::setprecision(2);

    std::cout << "Total memory:      " << total_mib << " MiB\n";
    std::cout << "Available memory:  " << available_mib << " MiB\n";
    std::cout << "Available memory:  " << available_percent << "%\n";

    std::cout << "\n--- Analysis ---\n";
    std::cout << "Pressure level:    " << pressure_name(pressure) << '\n';
    std::cout << "Reclamation mode:  " << mode_name(decision.mode) << '\n';
    std::cout << "Policy reason:     " << decision.reason << '\n';

    std::cout << "\n--- Controller ---\n";
    std::cout << result.message << '\n';
    std::cout << "Action performed:  "
              << (result.action_taken ? "Yes" : "No") << '\n';

    std::cout << "===============================\n";

    return 0;
}
