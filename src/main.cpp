
#include <iostream>

#include "ghostbyte/types.hpp"

int main() {
    std::cout << "GhostByte v0.1.0\n";
    std::cout << "Workload-Aware Adaptive Memory Reclamation\n";
    std::cout << "Platform: Windows\n";
    std::cout << "Status: Core foundation initialized\n";

    const ghostbyte::MemorySnapshot snapshot;
    const ghostbyte::PolicyDecision decision;

    std::cout << "\nMonitoring initialized: "
              << (snapshot.valid ? "yes" : "no") << '\n';

    std::cout << "Current pressure assessment: "
              << (decision.pressure == ghostbyte::PressureLevel::Unknown
                      ? "Unknown"
                      : "Initialized")
              << '\n';

    return 0;
}
