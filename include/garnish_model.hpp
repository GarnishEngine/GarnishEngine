#pragma once
#include "garnish_mesh.hpp"
#include "garnish_texture.hpp"

namespace garnish {
    class model {
        public:
            model(std::string modelPath) : modelPath(modelPath) {
                loadModel(modelPath);
            }   
            void draw();
        private:
            std::vector<Mesh> meshes;
            std::string modelPath;
            
            std::vector<g_texture> materialTextures(std::vector<std::string> materialPaths);

            void loadModel(std::string modelPath);
    };
}