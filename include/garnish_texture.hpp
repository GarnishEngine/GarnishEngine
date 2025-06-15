#pragma once
#include <string>
#include "OpenGL.hpp"
#include "stb_image.h"
#include <iostream>

namespace garnish {
    class g_texture {
    public:
        ~g_texture() { if (valid) stbi_image_free(data); };

        void load_texture(const std::string& texturePath);
        GLuint texture = -1;
    private: 
        int width, height, nrChannels;
        unsigned char *data;
        bool valid = false;
    };
}