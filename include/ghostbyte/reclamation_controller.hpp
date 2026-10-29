#ifndef GHOSTBYTE_RECLAMATION_CONTROLLER_HPP
#define GHOSTBYTE_RECLAMATION_CONTROLLER_HPP

#include "ghostbyte/types.hpp"

#include <cstdint>
#include <string>

namespace ghostbyte {

struct ReclamationResult {
    bool action_taken = false;
    bool measurement_valid = false;
    ReclamationMode mode = ReclamationMode::None;
    std::uint64_t memory_before_bytes = 0;
    std::uint64_t memory_after_bytes = 0;
    std::uint64_t observed_reclaimed_bytes = 0;
    std::string message;
};

struct ReclamationConfig {
    std::string cgroup_path;
    std::uint64_t reclaim_bytes = 0;
    std::uint64_t measurement_delay_ms = 100;
    bool enabled = false;
};

class ReclamationController {
public:
    explicit ReclamationController(
        ReclamationConfig config = {});

    ReclamationResult execute(
        const PolicyDecision& decision,
        bool dry_run = true) const;

private:
    ReclamationConfig config_;
};

}

#endif
