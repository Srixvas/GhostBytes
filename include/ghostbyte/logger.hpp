#ifndef GHOSTBYTE_LOGGER_HPP
#define GHOSTBYTE_LOGGER_HPP

#include <string>

namespace ghostbyte {

class Logger {
public:
    explicit Logger(const std::string& file_path = "logs/ghostbyte.log");

    bool log(const std::string& message) const;

private:
    std::string file_path_;
};

}

#endif
