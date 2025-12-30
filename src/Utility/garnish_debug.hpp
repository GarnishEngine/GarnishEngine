#pragma once

#include <glm/vec3.hpp>
#include <iostream>
#include <string>

namespace garnish {
inline std::string to_string(const glm::vec3& v) {
    return std::to_string(v.x) + " " + std::to_string(v.y) + " " +
           std::to_string(v.z);
}

inline std::ostream& operator<<(std::ostream& os, const glm::vec3& v) {
    return os << to_string(v);
}

inline void debug(const std::string& message) {
    std::cerr << message << '\n';
}

inline void debug(const glm::vec3& vec) {
    std::cerr << to_string(vec) << '\n';
}
}  // namespace garnish