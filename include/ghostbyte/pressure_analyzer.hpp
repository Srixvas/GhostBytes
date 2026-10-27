#ifndef GHOSTBYTE_PRESSURE_ANALYZER_HPP
#define GHOSTBYTE_PRESSURE_ANALYZER_HPP

#include "ghostbyte/config.hpp"
#include "ghostbyte/types.hpp"

namespace ghostbyte {

class PressureAnalyzer {
public:
    explicit PressureAnalyzer(const Config& config);

    PressureLevel analyze(const MemorySnapshot& snapshot) const;

private:
    Config config_;
};

}

#endif
