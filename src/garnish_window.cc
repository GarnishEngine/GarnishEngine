#include "garnish_window.hpp"
#include "graphics_pipeline.hpp"

#include <GL/glew.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_opengl.h>

namespace garnish {
    GarnishWindow::GarnishWindow(uint32_t w, uint32_t h, std::string name) 
        : width(w), height(h), windowName(name) {
        SDLFlags = (SDL_INIT_VIDEO | SDL_INIT_EVENTS);
        
        #ifdef _VULKAN_RENDERING
        windowFlags = (SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY |
                       SDL_WINDOW_RESIZABLE);
        #else
        windowFlags = (SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY |
                       SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
        #endif

        InitWindow();
    }

    GarnishWindow::GarnishWindow(uint32_t w, uint32_t h, std::string name,
                               uint64_t sdl_flags, uint64_t window_flags) 
        : width(w),
        height(h),
        windowName(name),
        SDLFlags(sdl_flags),
        windowFlags(window_flags) {
      InitWindow();
    }

    GarnishWindow::~GarnishWindow() {
        SDL_GL_DestroyContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void GarnishWindow::InitWindow() {
        SDL_Init(SDLFlags);

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        window = SDL_CreateWindow(windowName.c_str(), width, height, windowFlags);
        // SDL_AddEventWatch(resizingEventWatcher, window); maybe later

        glContext = SDL_GL_CreateContext(window);
    }

    void GarnishWindow::SwapWindow() {
        SDL_GL_SwapWindow(window);
    }
}
    