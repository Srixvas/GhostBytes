#ifndef GHOSTBYTE_CONFIG_HPP
#define GHOSTBYTE_CONFIG_HPP

namespace ghostbyte {

struct Config {
    double critical_threshold_percent = 10.0;
    double elevated_threshold_percent = 20.0;

    bool is_valid() const;
};

}

#endif
