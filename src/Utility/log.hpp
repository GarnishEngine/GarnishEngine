#pragma once

#include <chrono>
#include <fstream>
#include <string_view>

namespace garnish {
inline std::ofstream& get_log() {
    static std::ofstream log_file("log.txt");
    return log_file;
}

inline void log_timed(std::string_view message) {
    using namespace std::chrono;
    const time_point now{system_clock::now()};
    const year_month_day ymd{floor<days>(now)};

    get_log() << "[" << ymd << "]" << message << '\n';
}

#ifndef NDEBUG
inline void log_debug(std::string_view message) {
    using namespace std::chrono;
    const time_point now{system_clock::now()};
    const year_month_day ymd{floor<days>(now)};

    get_log() << "[DEBUG][" << ymd << "]" << message << '\n';
}
#else
inline void log_debug(std::string_view message) {
    (void)message;  // Suppress unused parameter warning
}
#endif
}  // namespace garnish