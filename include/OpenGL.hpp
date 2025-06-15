#pragma once  
// glew.h Must be included before SDL_opengl.h

#include <cstddef>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#define GLEW_STATIC
#define GL_PTR_OFFSET(i) reinterpret_cast<void*>(static_cast<intptr_t>(i))

#include <GL/glew.h>

#include <SDL3/SDL_opengl.h>

