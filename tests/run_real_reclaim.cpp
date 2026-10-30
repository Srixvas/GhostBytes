#include "ghostbyte/reclamation_controller.hpp"
#include "ghostbyte/policy_engine.hpp"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <cgroup_path> <bytes_to_reclaim>\n";
        return 1;
    }

    ghostbyte::ReclamationConfig config;
    config.enabled = true; // Enable real execution
    config.cgroup_path = argv[1];
    config.reclaim_bytes = std::stoull(argv[2]);
    config.measurement_delay_ms = 500; // Allow 500ms for kernel to drop caches

    ghostbyte::ReclamationController controller(config);
    
    // Simulate an aggressive decision
    ghostbyte::PolicyDecision decision;
    decision.pressure = ghostbyte::PressureLevel::Critical;
    decision.mode = ghostbyte::ReclamationMode::Aggressive;
    decision.reason = "Integration Test Real Reclamation";

    auto result = controller.execute(decision, false);

    std::cout << "Action Taken       : " << (result.action_taken ? "Yes" : "No") << '\n';
    std::cout << "Measurement Valid  : " << (result.measurement_valid ? "Yes" : "No") << '\n';
    std::cout << "Memory Before      : " << result.memory_before_bytes << " bytes\n";
    std::cout << "Memory After       : " << result.memory_after_bytes << " bytes\n";
    std::cout << "Observed Reclaimed : " << result.observed_reclaimed_bytes << " bytes\n";
    std::cout << "Controller Message : " << result.message << '\n';

    return result.action_taken ? 0 : 1;
}
