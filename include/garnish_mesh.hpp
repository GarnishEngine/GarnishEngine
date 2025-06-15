#pragma once
#include "OpenGL.hpp"

#include "garnish_texture.hpp"
#include "stb_image.h"
#include <cstdlib>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <string>
#include <vector>

namespace garnish {
    struct vertex3d {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec2 texCoord;
        vertex3d() = default;
        vertex3d(glm::vec3 pos, glm::vec3 color) : pos(pos), color(color) {}
        vertex3d(glm::vec3 pos, glm::vec3 color, glm::vec2 texCoord) : pos(pos), color(color), texCoord(texCoord) {}
    };

    struct base_texture {
        unsigned int id;
        std::string type;
    };

    class Mesh {
        public: 
            std::vector<vertex3d> vertices;
            std::vector<uint32_t> indices;
            std::vector<base_texture> textures;

            Mesh() = default;
            Mesh(std::vector<vertex3d> Vertices,
                                    std::vector<uint32_t> indices,
                                    std::vector<base_texture> textures);
            Mesh(std::vector<vertex3d> vVertices,
                        std::vector<uint32_t> indices);

            void setup_mesh();
            void delete_vertex_array();

            void load_mesh(const std::string& mesh_path);
            void draw();
            void load_texture(const std::string& texture_path);
            void load_texture(const g_texture& texture);

        private: 
            uint32_t VAO = 0,
                     VBO = 0,
                     EBO = 0;
            g_texture gTexture;

    };
    
}
