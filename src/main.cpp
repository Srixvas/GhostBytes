
#include "ghostbyte/system_monitor.hpp"
#include "ghostbyte/pressure_analyzer.hpp"
#include "ghostbyte/policy_engine.hpp"
#include "ghostbyte/reclamation_controller.hpp"
#include "ghostbyte/logger.hpp"
#include "ghostbyte/config.hpp"
#include "ghostbyte/feedback_analyzer.hpp"

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
        case ghostbyte::ReclamationMode::None:         return "None";
        case ghostbyte::ReclamationMode::Conservative: return "Conservative";
        case ghostbyte::ReclamationMode::Moderate:     return "Moderate";
        case ghostbyte::ReclamationMode::Aggressive:   return "Aggressive";
    }

    return "Unknown";
}

const char* memory_trend_name(ghostbyte::MemoryTrend trend) {
    using ghostbyte::MemoryTrend;

    switch (trend) {
        case MemoryTrend::Increased: return "Increased";
        case MemoryTrend::Decreased: return "Decreased";
        case MemoryTrend::Unchanged: return "Unchanged";
        case MemoryTrend::Unknown:   return "Unknown";
    }

    return "Unknown";
}

const char* pressure_trend_name(ghostbyte::PressureTrend trend) {
    using ghostbyte::PressureTrend;

    switch (trend) {
        case PressureTrend::Improved:   return "Improved";
        case PressureTrend::Worsened:   return "Worsened";
        case PressureTrend::Unchanged:  return "Unchanged";
        case PressureTrend::Unknown:    return "Unknown";
    }

    return "Unknown";
}

double bytes_to_mib(std::uint64_t bytes) {
    return static_cast<double>(bytes) / (1024.0 * 1024.0);
}

}

int main(int argc, char** argv) {
    using namespace ghostbyte;

    Config config;
    bool dry_run = true;
    std::string target_cgroup;
    if (argc == 3 && std::string(argv[1]) == "--test-cgroup") {
        target_cgroup = argv[2];
        dry_run = false;
    }
    
    if (const char* crit = std::getenv("GB_CRITICAL")) {
        config.critical_threshold_percent = std::stod(crit);
    }
    if (const char* elev = std::getenv("GB_ELEVATED")) {
        config.elevated_threshold_percent = std::stod(elev);
    }


    if (!config.is_valid()) {
        std::cerr << "Invalid GhostBytes configuration.\n";
        return 1;
    }

    std::signal(SIGINT, handle_sigint);

    SystemMonitor monitor;
    PressureAnalyzer analyzer(config);
    PolicyEngine engine;
    ReclamationConfig rec_config;
    rec_config.enabled = !dry_run;
    rec_config.cgroup_path = target_cgroup;
    ReclamationController controller(rec_config);
    Logger logger;
    FeedbackAnalyzer feedback_analyzer;

    bool has_previous_snapshot = false;
    MemorySnapshot previous_snapshot;
    PressureLevel previous_pressure = PressureLevel::Unknown;
    ReclamationFeedback previous_reclamation_feedback;

    

    std::uint64_t cycle = 0;

    while (!shutdown_requested) {
        ++cycle;

        const MemorySnapshot snapshot = monitor.capture();
        const auto processes = monitor.capture_processes();
        const PressureLevel pressure = analyzer.analyze(snapshot);

        FeedbackResult feedback;

        if (has_previous_snapshot) {
            feedback = feedback_analyzer.analyze(
                previous_snapshot,
                snapshot,
                previous_pressure,
                pressure
            );
        }

        const PolicyDecision decision = engine.decide(
            pressure,
            processes,
            feedback,
            previous_reclamation_feedback);

        const ReclamationResult result = controller.execute(decision, dry_run);

        // Dry runs and unmeasured actions are not reclamation outcomes.
        ReclamationFeedback current_reclamation_feedback;
        if (result.action_taken && result.measurement_valid) {
            current_reclamation_feedback.valid = true;
            current_reclamation_feedback.observed_reclaimed_bytes =
                result.observed_reclaimed_bytes;
        }

        bool logging_ok = true;
        logging_ok &= logger.log("Cycle: " + std::to_string(cycle));

        double available_percent = 0.0;

        if (snapshot.valid && snapshot.total_memory_bytes > 0) {
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

        if (feedback.valid) {
            logging_ok &= logger.log(
                "Feedback: available memory delta=" +
                std::to_string(feedback.available_memory_delta_bytes) +
                " bytes, memory trend=" +
                std::string(memory_trend_name(feedback.memory_trend)) +
                ", pressure trend=" +
                std::string(pressure_trend_name(feedback.pressure_trend)));
        } else {
            logging_ok &= logger.log(
                "Feedback: unavailable (baseline or invalid measurements)");
        }

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

        
        const char* CLEAR_SCREEN = "\033[2J\033[H";
        const char* RESET = "\033[0m";
        const char* RED = "\033[1;31m";
        const char* YELLOW = "\033[1;33m";
        const char* GREEN = "\033[1;32m";
        const char* CYAN = "\033[1;36m";
        const char* BOLD = "\033[1m";

        const char* pressure_color = GREEN;
        if (pressure == ghostbyte::PressureLevel::Critical) pressure_color = RED;
        else if (pressure == ghostbyte::PressureLevel::Elevated) pressure_color = YELLOW;

        std::cout << CLEAR_SCREEN;
        std::cout << BOLD << CYAN << "========== GhostBytes Live Dashboard ==========" << RESET << "\n";
        std::cout << "Cycle: " << cycle << " | Mode: " << (dry_run ? "DRY RUN (safe mode)" : "LIVE RECLAMATION (test target)") << "\n";
        std::cout << "-----------------------------------------------\n";

        if (snapshot.valid && snapshot.total_memory_bytes > 0) {
            std::cout << BOLD << "System Memory : " << RESET
                      << std::fixed << std::setprecision(2)
                      << bytes_to_mib(snapshot.available_memory_bytes) << " / "
                      << bytes_to_mib(snapshot.total_memory_bytes) << " MiB available ("
                      << available_percent << "%)\n";
        } else {
            std::cout << BOLD << "System Memory : " << RESET << "unavailable\n";
        }

        std::cout << BOLD << "Pressure Level: " << RESET << pressure_color << pressure_name(pressure) << RESET << "\n";
        std::cout << BOLD << "Active Mode   : " << RESET << mode_name(decision.mode) << "\n";
        std::cout << BOLD << "Decision      : " << RESET << decision.reason << "\n";
        std::cout << "-----------------------------------------------\n";

        if (decision.candidate_pid > 0) {
            std::cout << BOLD << "[Target Workload] " << RESET << "PID " << decision.candidate_pid
                      << " (" << decision.candidate_name << ") | RSS: "
                      << bytes_to_mib(decision.candidate_rss_bytes) << " MiB\n";
        } else {
            std::cout << BOLD << "[Target Workload] " << RESET << "None\n";
        }

        std::cout << BOLD << "[Controller]      " << RESET << result.message << "\n";
        std::cout << "-----------------------------------------------\n";

        if (feedback.valid) {
            const double delta_mib = static_cast<double>(feedback.available_memory_delta_bytes) / (1024.0 * 1024.0);
            std::cout << BOLD << "[Feedback]        " << RESET << "Avail Change: " << std::showpos << delta_mib << " MiB" << std::noshowpos
                      << " | Trend: " << memory_trend_name(feedback.memory_trend)
                      << " | Pressure: " << pressure_trend_name(feedback.pressure_trend) << "\n";
        } else {
            std::cout << BOLD << "[Feedback]        " << RESET << "Awaiting baseline measurements...\n";
        }

        std::cout << "===============================================\n";
        std::cout << "Press Ctrl+C to stop.\n";
        std::cout << std::flush;

        if (!logging_ok) {
            std::cerr
                << "Warning: One or more log entries could not be written.\n";
        }

        previous_snapshot = snapshot;
        previous_pressure = pressure;
        has_previous_snapshot = snapshot.valid &&
                        pressure != PressureLevel::Unknown;

        previous_reclamation_feedback = current_reclamation_feedback;

        for (unsigned int tick = 0;
             tick < config.sampling_interval_seconds * 10U &&
             !shutdown_requested;
             ++tick) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    std::cout << "\nShutdown requested. GhostBytes stopped cleanly.\n";

    logger.log("GhostBytes stopped cleanly after " +
               std::to_string(cycle) + " monitoring cycle(s).");

    return 0;
}
