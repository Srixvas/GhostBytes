#include <cstdint>
#include <iostream>

#include "ghostbyte/policy_engine.hpp"
#include "ghostbyte/pressure_analyzer.hpp"
#include "ghostbyte/system_monitor.hpp"
#include "ghostbyte/types.hpp"

int main() {
    std::cout << "GhostByte v0.1.0\n";
    std::cout << "Workload-Aware Adaptive Memory Reclamation\n";

#if defined(_WIN32)
    std::cout << "Platform: Windows\n";
#elif defined(__linux__)
    std::cout << "Platform: Linux\n";
#else
    std::cout << "Platform: Unknown\n";
#endif

    const ghostbyte::SystemMonitor monitor;
    const ghostbyte::PressureAnalyzer analyzer;
    const ghostbyte::PolicyEngine policy_engine;

    const ghostbyte::MemorySnapshot snapshot = monitor.capture();
    const ghostbyte::PressureLevel pressure = analyzer.analyze(snapshot);
    const ghostbyte::PolicyDecision decision =
        policy_engine.decide(pressure);

    std::cout << "\nMonitoring initialized: "
              << (snapshot.valid ? "yes" : "no") << '\n';

    if (snapshot.valid) {
        constexpr std::uint64_t bytes_per_mib = 1024 * 1024;

        std::cout << "Total memory: "
                  << snapshot.total_memory_bytes / bytes_per_mib
                  << " MiB\n";

        std::cout << "Available memory: "
                  << snapshot.available_memory_bytes / bytes_per_mib
                  << " MiB\n";

        std::cout << "Total swap: "
                  << snapshot.swap_total_bytes / bytes_per_mib
                  << " MiB\n";

        std::cout << "Free swap: "
                  << snapshot.swap_free_bytes / bytes_per_mib
                  << " MiB\n";

        const double available_percent =
            (static_cast<double>(snapshot.available_memory_bytes) /
             static_cast<double>(snapshot.total_memory_bytes)) * 100.0;

        std::cout << "Available memory percentage: "
                  << available_percent << "%\n";
    }

    std::cout << "Current pressure assessment: ";

    switch (pressure) {
        case ghostbyte::PressureLevel::Healthy:
            std::cout << "Healthy\n";
            break;
        case ghostbyte::PressureLevel::Elevated:
            std::cout << "Elevated\n";
            break;
        case ghostbyte::PressureLevel::Critical:
            std::cout << "Critical\n";
            break;
        case ghostbyte::PressureLevel::Unknown:
            std::cout << "Unknown\n";
            break;
    }

    std::cout << "Selected reclamation mode: ";

    switch (decision.mode) {
        case ghostbyte::ReclamationMode::None:
            std::cout << "None\n";
            break;
        case ghostbyte::ReclamationMode::Conservative:
            std::cout << "Conservative\n";
            break;
        case ghostbyte::ReclamationMode::Moderate:
            std::cout << "Moderate\n";
            break;
        case ghostbyte::ReclamationMode::Aggressive:
            std::cout << "Aggressive\n";
            break;
    }

    std::cout << "Decision reason: " << decision.reason << '\n';

    return snapshot.valid ? 0 : 1;
}
