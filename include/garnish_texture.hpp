#pragma once
#include <iostream>
#include <string>

#include "OpenGL.hpp"
#include "stb_image.h"

namespace garnish {
class Texture {
   public:
    bool load_texture(const std::string& texturePath);
    void bind();
    void unbind();
    void cleanup();

   private:
    GLuint texture = -1;
};
}  // namespace garnish