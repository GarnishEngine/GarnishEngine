#include "garnish_sprite.hpp"
#include "OpenGL.hpp"
#include "gl_buffer.hpp"
#include "garnish_texture.hpp"
#include <cstdint>
#include "stb_image.h"

#include <tiny_obj_loader.h>

namespace garnish {
    void Sprite::setup_sprite() {
        glGenBuffers(1, &VBO);
        glGenVertexArrays(1, &VAO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex2d),
                    &vertices[0], GL_STATIC_DRAW);

        // vertex2d positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex2d),
                                (void *)0);
        // vertex2d color
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex2d),
                                (void *)offsetof(vertex2d, color));
        // vertex2d texture coords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex2d),
                                (void *)offsetof(vertex2d, texCoord));

        glBindVertexArray(0);
    }

    void Sprite::load_texture(const std::string& texture_path) {
        gTexture.load_texture(texture_path);
        glBindTexture(GL_TEXTURE_2D, gTexture.texture);
        glBindVertexArray(VAO);
        glBindVertexArray(0);

    }
    void Sprite::load_texture(g_texture texture) {
        glBindTexture(GL_TEXTURE_2D, texture.texture);
        glBindVertexArray(VAO);
        glBindVertexArray(0);

    }

    void Sprite::draw() {

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

    void Sprite::delete_vertex_array() {
        glDeleteVertexArrays(1, &VAO);
    }
}