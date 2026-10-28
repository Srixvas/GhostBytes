#ifndef GHOSTBYTE_CONFIG_HPP
#define GHOSTBYTE_CONFIG_HPP

namespace ghostbyte {

struct Config {
    double critical_threshold_percent = 10.0;
    double elevated_threshold_percent = 20.0;
    unsigned int sampling_interval_seconds = 2;

    bool is_valid() const;
};

}

#endif
