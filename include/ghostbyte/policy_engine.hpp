#ifndef GHOSTBYTE_POLICY_ENGINE_HPP
#define GHOSTBYTE_POLICY_ENGINE_HPP

#include "ghostbyte/types.hpp"

namespace ghostbyte {

class PolicyEngine {
public:
    PolicyDecision decide(PressureLevel pressure) const;
};

}

#endif
