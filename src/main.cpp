#include "ghostbyte/system_monitor.hpp"
#include "ghostbyte/pressure_analyzer.hpp"
#include "ghostbyte/policy_engine.hpp"
#include "ghostbyte/reclamation_controller.hpp"
#include "ghostbyte/logger.hpp"
#include "ghostbyte/config.hpp"

#include <iomanip>
#include <iostream>
#include <string>

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
        case ReclamationMode::Aggressive:   return "Aggressive";
    }

    return "Unknown";
}

}

int main() {
    using namespace ghostbyte;

    Config config;

    if (!config.is_valid()) {
        std::cerr << "Invalid GhostBytes configuration.\n";
        return 1;
    }

    SystemMonitor monitor;
    PressureAnalyzer analyzer(config);
    PolicyEngine engine;
    ReclamationController controller;
    Logger logger;

    MemorySnapshot snapshot = monitor.capture();

    if (!snapshot.valid) {
        std::cerr << "Failed to collect valid memory statistics.\n";

        if (!logger.log("ERROR: Failed to collect valid memory statistics")) {
            std::cerr << "Warning: Could not write to the log file.\n";
        }

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

    bool logging_ok = true;

    logging_ok &= logger.log(
        "Total memory: " + std::to_string(total_mib) + " MiB");

    logging_ok &= logger.log(
        "Available memory: " + std::to_string(available_mib) + " MiB");

    logging_ok &= logger.log(
        "Available memory: " + std::to_string(available_percent) + "%");

    logging_ok &= logger.log(
        "Pressure level: " + std::string(pressure_name(pressure)));

    logging_ok &= logger.log(
        "Reclamation mode: " + std::string(mode_name(decision.mode)));

    logging_ok &= logger.log(
        "Policy reason: " + std::string(decision.reason));

    logging_ok &= logger.log(
        "Controller result: " + result.message);

    if (!logging_ok) {
        std::cerr << "Warning: One or more log entries could not be written.\n";
    }

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
