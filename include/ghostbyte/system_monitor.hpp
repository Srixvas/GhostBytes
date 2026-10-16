#ifndef GHOSTBYTE_SYSTEM_MONITOR_HPP
#define GHOSTBYTE_SYSTEM_MONITOR_HPP

#include "ghostbyte/types.hpp"

namespace ghostbyte {

class SystemMonitor {
public:
    MemorySnapshot capture() const;
};

}

#endif
