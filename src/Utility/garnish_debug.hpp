#pragma once

#include <format>
#include <glm/vec3.hpp>
#include <iostream>
#include <string>

namespace garnish {
[[nodiscard]] inline std::string to_string(const glm::vec3& v) noexcept {
    return std::format("{} {} {}", v.x, v.y, v.z);
}

inline std::ostream& operator<<(std::ostream& os, const glm::vec3& v) noexcept {
    return os << to_string(v);
}

inline void debug(const std::string& message) noexcept {
    std::cerr << message << '\n';
}

inline void debug(const glm::vec3& vec) noexcept {
    std::cerr << to_string(vec) << '\n';
}
}  // namespace garnish