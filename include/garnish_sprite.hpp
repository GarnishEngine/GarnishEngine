#pragma once
#include <cstdlib>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <string>
#include <vector>

#include "OpenGL.hpp"
#include "garnish_texture.hpp"
#include "gl_buffer.hpp"
#include "glm/ext/vector_float3.hpp"
#include "stb_image.h"

namespace garnish {
struct vertex2d {
    glm::vec2 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
    vertex2d() {}
    vertex2d(glm::vec2 pos, glm::vec3 color)
        : pos(pos),
          color(color) {}
    vertex2d(glm::vec2 pos, glm::vec3 color, glm::vec2 texCoord)
        : pos(pos),
          color(color),
          texCoord(texCoord) {}
};

class Sprite {
   public:
    Sprite() {}
    Sprite(Texture texture)
        : gTexture(texture) {}
    Sprite(const std::string& texture_path) {
        gTexture.load_texture(texture_path);
    }

    void delete_vertex_array();
    void draw();
    void setup_sprite();
    void load_texture(const std::string& texture_path);
    void load_texture(Texture texture);

   private:
    uint32_t VAO, VBO;
    std::vector<vertex2d> vertices = {
        // pos      // tex
        vertex2d{
            glm::vec2(0.0F, 1.0F),
            glm::vec3(1.0F, 1.0F, 1.0F),
            glm::vec2(0.0F, 1.0F),
        },
        vertex2d{
            glm::vec2(1.0F, 0.0F),
            glm::vec3(1.0F, 1.0F, 1.0F),
            glm::vec2(1.0F, 0.0F)
        },
        vertex2d{
            glm::vec2(0.0F, 0.0F),
            glm::vec3(1.0F, 1.0F, 1.0F),
            glm::vec2(0.0F, 0.0F)
        },

        vertex2d{
            glm::vec2(0.0F, 1.0F),
            glm::vec3(1.0F, 1.0F, 1.0F),
            glm::vec2(0.0F, 1.0F)
        },
        vertex2d{
            glm::vec2(1.0F, 1.0F),
            glm::vec3(1.0F, 1.0F, 1.0F),
            glm::vec2(1.0F, 1.0F)
        },
        vertex2d{
            glm::vec2(1.0F, 0.0F),
            glm::vec3(1.0F, 1.0F, 1.0F),
            glm::vec2(1.0F, 0.0F)
        }
    };

    Texture gTexture;
};

}  // namespace garnish
