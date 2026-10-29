#ifndef GHOSTBYTE_FEEDBACK_ANALYZER_HPP
#define GHOSTBYTE_FEEDBACK_ANALYZER_HPP

#include "ghostbyte/types.hpp"

namespace ghostbyte {

class FeedbackAnalyzer {
public:
    FeedbackResult analyze(
        const MemorySnapshot& before,
        const MemorySnapshot& after,
        PressureLevel pressure_before,
        PressureLevel pressure_after) const;
};

}

#endif
