#pragma once

#include "OpenGL/OpenGL.hpp"
#include "OpenGL/gl_buffer.hpp"
#include "garnish_texture.hpp"
#include "stb_image.h"
#include <cstdlib>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <string>
#include <vector>

namespace garnish {
    struct vertex {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec2 texCoord;
        vertex() {}
        vertex(glm::vec3 pos, glm::vec3 color) : pos(pos), color(color) {}
        vertex(glm::vec3 pos, glm::vec3 color, glm::vec2 texCoord) : pos(pos), color(color), texCoord(texCoord) {}
    };

    struct base_texture {
        unsigned int id;
        std::string type;
    };

    class mesh {
        public: 
            std::vector<vertex> vertices;
            std::vector<uint32_t> indices;
            std::vector<base_texture> textures;

            mesh() {}
            mesh(std::vector<vertex> Vertices,
                                    std::vector<uint32_t> indices,
                                    std::vector<base_texture> textures);
            mesh(std::vector<vertex> vVertices,
                        std::vector<uint32_t> indices);

            void setupMesh();
            void deleteVertexArray();

            void loadMesh(std::string meshPath);
            void draw();
            void loadTexture(std::string texturePath);
        private: 
            uint32_t VAO, VBO, EBO;
            garnish_texture gTexture;

    };
    
}
