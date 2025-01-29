#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <iostream>
#include <string>

namespace garnish {
    void debug(std::string message) {
        std::cerr << message << '\n';
    }
    void debug(glm::vec3 vec) {
      std::cerr << std::to_string(vec.x) << " " << std::to_string(vec.y) << " "
                << std::to_string(vec.z) << '\n';
    }
} 