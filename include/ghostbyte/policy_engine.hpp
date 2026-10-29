#ifndef GHOSTBYTE_POLICY_ENGINE_HPP
#define GHOSTBYTE_POLICY_ENGINE_HPP

#include "ghostbyte/types.hpp"
#include <vector>

namespace ghostbyte {

class PolicyEngine {
public:
    PolicyDecision decide(PressureLevel pressure) const;

    PolicyDecision decide(
        PressureLevel pressure,
        const std::vector<ProcessSnapshot>& processes) const;

    PolicyDecision decide(
        PressureLevel pressure,
        const std::vector<ProcessSnapshot>& processes,
        const FeedbackResult& feedback) const;
};

}

#endif
