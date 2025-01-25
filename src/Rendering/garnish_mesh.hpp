#pragma once 

#include "OpenGL/OpenGL.hpp"
#include <string>
#include <vector>

namespace garnish {
    struct vertex {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec2 texCoord;
    };

    struct texture {
        unsigned int id;
        std::string type;
    };

    class GarnishMesh {
        public: 
            std::vector<vertex> verticies;
            std::vector<int32_t> indicies;
            std::vector<texture> textures;
        private:
        
    };
    
}
