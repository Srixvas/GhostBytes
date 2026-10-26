#ifndef GHOSTBYTE_RECLAMATION_CONTROLLER_HPP
#define GHOSTBYTE_RECLAMATION_CONTROLLER_HPP

#include "ghostbyte/types.hpp"

#include <string>

namespace ghostbyte {

struct ReclamationResult {
    bool action_taken = false;
    ReclamationMode mode = ReclamationMode::None;
    std::string message;
};

class ReclamationController {
public:
    ReclamationResult execute(
        const PolicyDecision& decision,
        bool dry_run = true) const;
};

}

#endif
