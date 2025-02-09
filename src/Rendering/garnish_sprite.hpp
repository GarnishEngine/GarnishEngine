#pragma once

#include "OpenGL/OpenGL.hpp"
#include "OpenGL/gl_buffer.hpp"
#include "garnish_texture.hpp"
#include "glm/ext/vector_float3.hpp"
#include "stb_image.h"
#include <cstdlib>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <string>
#include <vector>

namespace garnish {
    struct vertex2d {
        glm::vec2 pos;
        glm::vec3 color;
        glm::vec2 texCoord;
        vertex2d() {}
        vertex2d(glm::vec2 pos, glm::vec3 color) : pos(pos), color(color) {}
        vertex2d(glm::vec2 pos, glm::vec3 color, glm::vec2 texCoord) : pos(pos), color(color), texCoord(texCoord) {}
    };

    class sprite {
        public: 
            sprite() {}
            sprite(garnish_texture gTexture) : gTexture(gTexture) { }
            sprite(std::string texturePath) { gTexture.loadTexture(texturePath); }

            void deleteVertexArray();
            void draw();
            void setupSprite();
            void loadTexture(std::string texturePath);
            void loadTexture(garnish_texture texture);

        private: 
            uint32_t VAO, VBO;
            std::vector<vertex2d> vertices = { 
                // pos      // tex
                vertex2d{glm::vec2(0.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(0.0f, 1.0f),},
                vertex2d{glm::vec2(1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(1.0f, 0.0f)},
                vertex2d{glm::vec2(0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(0.0f, 0.0f)},
            
                vertex2d{glm::vec2(0.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(0.0f, 1.0f)},
                vertex2d{glm::vec2(1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(1.0f, 1.0f)},
                vertex2d{glm::vec2(1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(1.0f, 0.0f)}
            };
        
            garnish_texture gTexture;

    };
    
}
