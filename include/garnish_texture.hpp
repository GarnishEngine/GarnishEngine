#pragma once
#include <iostream>
#include <string>

#include "OpenGL.hpp"

namespace garnish {
class Texture {
   public:
    void bind() { glBindTexture(GL_TEXTURE_2D, texture); }
    void unbind() { glBindTexture(GL_TEXTURE_2D, 0); }

   private:
    GLuint texture = -1;
};
}  // namespace garnish