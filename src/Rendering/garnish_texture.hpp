#pragma once

#include <string>

#include "OpenGL/OpenGL.hpp"
#include "stb_image.h"
#include <iostream>

namespace garnish {
    class garnish_texture {
        public:
        garnish_texture() : valid(false) {};

        ~garnish_texture() { if (valid) stbi_image_free(data); };

        void loadTexture(std::string texturePath);
        int texture = -1;
        private: 
        int width, height, nrChannels;
        unsigned char *data;
        bool valid = false;
    };
}