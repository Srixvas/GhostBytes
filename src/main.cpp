#include "ghostbyte/system_monitor.hpp"
#include "ghostbyte/pressure_analyzer.hpp"
#include "ghostbyte/policy_engine.hpp"
#include "ghostbyte/reclamation_controller.hpp"
#include "ghostbyte/logger.hpp"
#include "ghostbyte/config.hpp"

#include <chrono>
#include <csignal>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>

namespace {

volatile std::sig_atomic_t shutdown_requested = 0;

void handle_sigint(int) {
    shutdown_requested = 1;
}

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

double bytes_to_mib(std::uint64_t bytes) {
    return static_cast<double>(bytes) / (1024.0 * 1024.0);
}

}

int main() {
    using namespace ghostbyte;

    Config config;

    if (!config.is_valid()) {
        std::cerr << "Invalid GhostBytes configuration.\n";
        return 1;
    }

    std::signal(SIGINT, handle_sigint);

    SystemMonitor monitor;
    PressureAnalyzer analyzer(config);
    PolicyEngine engine;
    ReclamationController controller;
    Logger logger;

    std::cout << "========== GhostBytes ==========\n"
              << "Sampling interval: "
              << config.sampling_interval_seconds << " second(s)\n"
              << "Mode: DRY RUN (no processes will be terminated)\n"
              << "Press Ctrl+C to stop.\n\n";

    std::uint64_t cycle = 0;

    while (!shutdown_requested) {
        ++cycle;

        const MemorySnapshot snapshot = monitor.capture();
        const auto processes = monitor.capture_processes();
        const PressureLevel pressure = analyzer.analyze(snapshot);
        const PolicyDecision decision = engine.decide(pressure, processes);
        const ReclamationResult result = controller.execute(decision, true);

        bool logging_ok = true;
        logging_ok &= logger.log("Cycle: " + std::to_string(cycle));

        double available_percent = 0.0;
        if (snapshot.valid) {
            available_percent =
                static_cast<double>(snapshot.available_memory_bytes) /
                static_cast<double>(snapshot.total_memory_bytes) * 100.0;

            logging_ok &= logger.log(
                "Memory: total=" +
                std::to_string(bytes_to_mib(snapshot.total_memory_bytes)) +
                " MiB, available=" +
                std::to_string(bytes_to_mib(snapshot.available_memory_bytes)) +
                " MiB (" + std::to_string(available_percent) + "%)");
        } else {
            logging_ok &= logger.log(
                "ERROR: Failed to collect valid system memory statistics");
        }

        logging_ok &= logger.log(
            "Process snapshot count: " + std::to_string(processes.size()));
        logging_ok &= logger.log(
            "Pressure level: " + std::string(pressure_name(pressure)));
        logging_ok &= logger.log(
            "Reclamation mode: " + std::string(mode_name(decision.mode)));
        logging_ok &= logger.log(
            "Policy reason: " + std::string(decision.reason));

        if (decision.candidate_pid > 0) {
            logging_ok &= logger.log(
                "Candidate: PID=" + std::to_string(decision.candidate_pid) +
                ", name=" + decision.candidate_name +
                ", RSS=" +
                std::to_string(bytes_to_mib(decision.candidate_rss_bytes)) +
                " MiB (assessment only; not terminated)");
        } else {
            logging_ok &= logger.log("Candidate: none");
        }

        logging_ok &= logger.log("Controller result: " + result.message);

        std::cout << "[Cycle " << cycle << "]\n";
        if (snapshot.valid) {
            std::cout << std::fixed << std::setprecision(2)
                      << "Memory: "
                      << bytes_to_mib(snapshot.available_memory_bytes)
                      << " / " << bytes_to_mib(snapshot.total_memory_bytes)
                      << " MiB available (" << available_percent << "%)\n";
        } else {
            std::cout << "Memory: unavailable (snapshot invalid)\n";
        }

        std::cout << "Processes observed: " << processes.size() << '\n'
                  << "Pressure: " << pressure_name(pressure) << '\n'
                  << "Mode: " << mode_name(decision.mode) << '\n'
                  << "Reason: " << decision.reason << '\n';

        if (decision.candidate_pid > 0) {
            std::cout << "Candidate: PID " << decision.candidate_pid
                      << " (" << decision.candidate_name << "), RSS "
                      << bytes_to_mib(decision.candidate_rss_bytes)
                      << " MiB — assessment only\n";
        } else {
            std::cout << "Candidate: none\n";
        }

        std::cout << "Controller: " << result.message << "\n\n";

        if (!logging_ok) {
            std::cerr
                << "Warning: One or more log entries could not be written.\n";
        }

        // Sleep in short increments so Ctrl+C is handled promptly.
        for (unsigned int tick = 0;
             tick < config.sampling_interval_seconds * 10U &&
             !shutdown_requested;
             ++tick) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    std::cout << "\nShutdown requested. GhostBytes stopped cleanly.\n";
    logger.log("GhostBytes stopped cleanly after " + std::to_string(cycle) +
               " monitoring cycle(s).");

    return 0;
}
