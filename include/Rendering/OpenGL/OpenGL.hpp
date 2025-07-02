#pragma once
#include <cstddef>
#include <shared.hpp>
#include <vector>

#define GLEW_STATIC
#define GL_PTR_OFFSET(i) reinterpret_cast<void*>(static_cast<intptr_t>(i))

// glew.h Must be included before SDL_opengl.h
#include <GL/glew.h>
#include <SDL3/SDL_opengl.h>

#include <string>

struct OGLVertex3d {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
    OGLVertex3d() = default;
};

struct OGLTexture {
    unsigned int id;
    std::string type;
};

using OGLMesh = std::vector<OGLVertex3d>;