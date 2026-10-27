#ifndef GHOSTBYTE_SYSTEM_MONITOR_HPP
#define GHOSTBYTE_SYSTEM_MONITOR_HPP

#include "ghostbyte/types.hpp"
#include <vector>

namespace ghostbyte {

class SystemMonitor {
public:
    MemorySnapshot capture() const;
    std::vector<ProcessSnapshot> capture_processes() const;
};

}

#endif
