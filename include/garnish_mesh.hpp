#pragma once
#include <cstdlib>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <string>
#include <vector>

#include "OpenGL.hpp"
#include "garnish_texture.hpp"
#include "stb_image.h"

namespace garnish {
struct Mesh {
   public:
    uint32_t VAO = 0, VBO = 0, EBO = 0, numIndicies = 0, texID = 0;
};

}  // namespace garnish
