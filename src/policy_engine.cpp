#include "ghostbyte/policy_engine.hpp"

namespace ghostbyte {

PolicyDecision PolicyEngine::decide(PressureLevel pressure) const {
    PolicyDecision decision;
    decision.pressure = pressure;

    switch (pressure) {
        case PressureLevel::Healthy:
            decision.mode = ReclamationMode::None;
            decision.reason = "Memory availability is healthy";
            break;

        case PressureLevel::Elevated:
            decision.mode = ReclamationMode::Conservative;
            decision.reason = "Memory pressure is elevated";
            break;

        case PressureLevel::Critical:
            decision.mode = ReclamationMode::Aggressive;
            decision.reason = "Memory pressure is critical";
            break;

        case PressureLevel::Unknown:
        default:
            decision.mode = ReclamationMode::None;
            decision.reason = "Memory pressure is unknown; no action recommended";
            break;
    }

    return decision;
}

}
