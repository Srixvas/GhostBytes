#ifndef GHOSTBYTE_PRESSURE_ANALYZER_HPP
#define GHOSTBYTE_PRESSURE_ANALYZER_HPP

#include "ghostbyte/types.hpp"

namespace ghostbyte {

class PressureAnalyzer {
public:
    PressureLevel analyze(const MemorySnapshot& snapshot) const;
};

}

#endif
