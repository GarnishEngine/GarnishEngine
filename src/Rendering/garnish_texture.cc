#include "garnish_texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace garnish {
bool Texture::load_texture(const std::string& texturePath) {}
void Texture::bind() {
    glBindTexture(GL_TEXTURE_2D, texture);
}
void Texture::unbind() {
    glBindTexture(GL_TEXTURE_2D, 0);
}
}  // namespace garnish
