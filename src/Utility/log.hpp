#pragma once

#include <fstream>

namespace garnish {
// Initialize log file for engine logging
inline std::ofstream& get_log() {
    static std::ofstream log_file("log.txt");
    return log_file;
}
}