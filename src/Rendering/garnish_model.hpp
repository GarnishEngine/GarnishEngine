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
            std::vector<mesh> meshes;
            std::string modelPath;
            
            std::vector<garnish_texture> materialTextures(std::vector<std::string> materialPaths);

            void loadModel(std::string modelPath);
    };
}