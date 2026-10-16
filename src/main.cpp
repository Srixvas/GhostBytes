#include <iostream>

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

    std::cout << "Status: Core foundation initialized\n";

    const ghostbyte::SystemMonitor monitor;
    const ghostbyte::MemorySnapshot snapshot = monitor.capture();
    const ghostbyte::PolicyDecision decision;

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
    }

    std::cout << "Current pressure assessment: ";

    switch (decision.pressure) {
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

    return snapshot.valid ? 0 : 1;
}