#include "garnish_sprite.hpp"
#include "OpenGL/OpenGL.hpp"
#include "OpenGL/gl_buffer.hpp"
#include "garnish_texture.hpp"
#include <cstdint>
#include "stb_image.h"


#include <tiny_obj_loader.h>

namespace garnish {
    void sprite::setupSprite() {
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

    void sprite::loadTexture(std::string texturePath) {
        gTexture.loadTexture(texturePath);
        glBindTexture(GL_TEXTURE_2D, gTexture.texture);
        glBindVertexArray(VAO);
        glBindVertexArray(0);

    }
    void sprite::loadTexture(garnish_texture gTexture) {
        glBindTexture(GL_TEXTURE_2D, gTexture.texture);
        glBindVertexArray(VAO);
        glBindVertexArray(0);

    }

    void sprite::draw() {

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

    void sprite::deleteVertexArray() {
        glDeleteVertexArrays(1, &VAO);
    }
}