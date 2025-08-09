#pragma once

#include <glm/vec3.hpp>
#include <iostream>
#include <string>

namespace garnish {
inline void debug(const std::string& message) {
    std::cerr << message << '\n';
}
inline void debug(const glm::vec3& vec) {
    std::cerr << std::to_string(vec.x) << " " << std::to_string(vec.y) << " "
              << std::to_string(vec.z) << '\n';
}
}  // namespace garnish