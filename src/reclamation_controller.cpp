#include "ghostbyte/reclamation_controller.hpp"

namespace ghostbyte {

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
                result.message =
                    "DRY RUN: Conservative reclamation planned";
                break;

            case ReclamationMode::Moderate:
                result.message =
                    "DRY RUN: Moderate reclamation planned";
                break;

            case ReclamationMode::Aggressive:
                result.message =
                    "DRY RUN: Aggressive reclamation planned";
                break;

            case ReclamationMode::None:
                result.message = "No reclamation required";
                break;
        }

        return result;
    }

    result.message =
        "Real reclamation is not implemented yet";
    return result;
}

}
