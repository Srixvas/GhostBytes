#include "ghostbyte/reclamation_controller.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>
#include <utility>
#include <unistd.h>
#include <cerrno>
#include <cstring>

namespace ghostbyte {
namespace {

bool read_memory_current(
    const std::filesystem::path& path,
    std::uint64_t& value) {

    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::uint64_t parsed_value = 0;
    if (!(file >> parsed_value)) {
        return false;
    }

    value = parsed_value;
    return true;
}

}

ReclamationController::ReclamationController(ReclamationConfig config)
    : config_(std::move(config)) {}

ReclamationResult ReclamationController::execute(
    const PolicyDecision& decision,
    bool dry_run) const {

    ReclamationResult result;
    result.mode = decision.mode;

    if (decision.pressure == PressureLevel::Unknown) {
        result.message = "Unknown memory pressure; no action taken";
        return result;
    }

    if (decision.mode == ReclamationMode::None) {
        result.message = "No reclamation required";
        return result;
    }

    if (dry_run) {
        switch (decision.mode) {
            case ReclamationMode::Conservative:
                result.message = "DRY RUN: Conservative reclamation planned";
                break;
            case ReclamationMode::Moderate:
                result.message = "DRY RUN: Moderate reclamation planned";
                break;
            case ReclamationMode::Aggressive:
                result.message = "DRY RUN: Aggressive reclamation planned";
                break;
            case ReclamationMode::None:
                result.message = "No reclamation required";
                break;
        }
        return result;
    }

    if (!config_.enabled) {
        result.message = "Real reclamation disabled by configuration";
        return result;
    }

    if (config_.cgroup_path.empty()) {
        result.message = "Reclamation rejected: cgroup path is empty";
        return result;
    }

    const std::uint64_t bytes_to_reclaim = decision.target_reclaim_bytes > 0 ? decision.target_reclaim_bytes : config_.reclaim_bytes;

    if (bytes_to_reclaim == 0 ||
        bytes_to_reclaim > 64ULL * 1024 * 1024) {
        result.message =
            "Reclamation rejected: request must be between 1 byte and 64 MiB";
        return result;
    }

    namespace fs = std::filesystem;

    std::error_code ec;
    const fs::path canonical_path =
        fs::canonical(config_.cgroup_path, ec);

    if (ec || !fs::is_directory(canonical_path)) {
        result.message =
            "Reclamation rejected: target is not an accessible directory";
        return result;
    }

    ec.clear();
    const fs::path cgroup_root = fs::canonical("/sys/fs/cgroup", ec);
    if (ec) {
        result.message =
            "Reclamation rejected: could not resolve cgroup filesystem root";
        return result;
    }

    const fs::path user_cgroup =
        fs::path("user.slice") /
        ("user-" + std::to_string(::getuid()) + ".slice") /
        ("user@" + std::to_string(::getuid()) + ".service");

    ec.clear();
    const fs::path delegated_root =
        fs::canonical(cgroup_root / user_cgroup.relative_path(), ec);

    if (ec || canonical_path == delegated_root ||
        canonical_path.string().compare(
            0, delegated_root.string().size(),
            delegated_root.string()) != 0 ||
        canonical_path.string().size() <= delegated_root.string().size() ||
        canonical_path.string()[delegated_root.string().size()] != '/') {
        result.message =
            "Reclamation rejected: target must be a child of the delegated user cgroup";
        return result;
    }

    const fs::path reclaim_file = canonical_path / "memory.reclaim";
    const fs::path current_file = canonical_path / "memory.current";

    if (!fs::exists(canonical_path / "cgroup.procs") ||
        !fs::exists(current_file) ||
        !fs::exists(reclaim_file) ||
        !fs::exists(canonical_path / "cgroup.controllers")) {
        result.message =
            "Reclamation rejected: required cgroup v2 interfaces are unavailable";
        return result;
    }

    if (!read_memory_current(current_file, result.memory_before_bytes)) {
        result.message =
            "Reclamation failed: could not read memory.current before action";
        return result;
    }

    {
        std::ofstream file(reclaim_file);
        if (!file.is_open()) {
            result.message =
                "Reclamation failed: could not open memory.reclaim";
            return result;
        }

        file << bytes_to_reclaim;
        file.flush();

        if (!file.good()) {
            result.message = std::string("Reclamation failed: could not write memory.reclaim (OS Error: ") + std::strerror(errno) + ")";
            return result;
        }
    }

    result.action_taken = true;

    std::this_thread::sleep_for(
        std::chrono::milliseconds(config_.measurement_delay_ms));

    if (!read_memory_current(current_file, result.memory_after_bytes)) {
        result.message =
            "Reclamation request submitted, but post-action measurement failed";
        return result;
    }

    result.measurement_valid = true;

    if (result.memory_before_bytes > result.memory_after_bytes) {
        result.observed_reclaimed_bytes =
            result.memory_before_bytes - result.memory_after_bytes;
    }

    result.message =
        "Reclamation request submitted; observed cgroup memory change measured";

    return result;
}

}
