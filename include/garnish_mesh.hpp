#pragma once
#include <cstdlib>
#include <string>
#include <vector>

#include "OpenGL.hpp"

namespace garnish {
struct Mesh {
   public:
    uint32_t VAO = 0, VBO = 0, EBO = 0, numIndicies = 0, texID = 0;
};

}  // namespace garnish
