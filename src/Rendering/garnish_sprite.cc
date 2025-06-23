#include "garnish_sprite.hpp"

#include <tiny_obj_loader.h>

#include <cstdint>

#include "OpenGL.hpp"
#include "garnish_texture.hpp"
#include "gl_buffer.hpp"
#include "stb_image.h"

namespace garnish {
void Sprite::setup_sprite() {
    glGenBuffers(1, &VBO);
    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(vertex2d),
        &vertices[0],
        GL_STATIC_DRAW
    );

    // vertex2d positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex2d), (void*)0);
    // vertex2d color
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(vertex2d),
        (void*)offsetof(vertex2d, color)
    );
    // vertex2d texture coords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(vertex2d),
        (void*)offsetof(vertex2d, texCoord)
    );

    glBindVertexArray(0);
}

void Sprite::load_texture(const std::string& texture_path) {
    gTexture.load_texture(texture_path);
    gTexture.bind();
}

void Sprite::load_texture(Texture texture) {
    gTexture.bind();
}

void Sprite::draw() {
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Sprite::delete_vertex_array() {
    glDeleteVertexArrays(1, &VAO);
}
}  // namespace garnish