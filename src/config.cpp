#include "ghostbyte/config.hpp"

#include <cmath>

namespace ghostbyte {

bool Config::is_valid() const {
    return std::isfinite(critical_threshold_percent) &&
           std::isfinite(elevated_threshold_percent) &&
           critical_threshold_percent >= 0.0 &&
           elevated_threshold_percent <= 100.0 &&
           critical_threshold_percent < elevated_threshold_percent;
}

}
