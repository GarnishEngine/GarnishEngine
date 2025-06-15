#include "garnish_window.hpp"

#include <utility>

namespace garnish {
    Window::Window(int32_t w, int32_t h, std::string name) 
        : width(w), 
          height(h), 
          SDLFlags(SDL_INIT_VIDEO | SDL_INIT_EVENTS),
          windowName(std::move(name)),
          windowFlags(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE), 
          sdl_window(init_window()), 
          glContext(init_context()) {        
        #ifdef _VULKAN_RENDERING
        windowFlags = (SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
        #else
        
        #endif
        glEnable(GL_DEPTH_TEST);

    }

    Window::Window(int32_t w, int32_t h, std::string name,
                               uint64_t sdl_flags, uint64_t window_flags) 
        : width(w),
        height(h),
        windowName(std::move(name)),
        SDLFlags(sdl_flags),
        windowFlags(window_flags),
        sdl_window(init_window()), 
        glContext(init_context()) {
        glEnable(GL_DEPTH_TEST);
    }

    Window::~Window() {
        SDL_GL_DestroyContext(glContext);
        SDL_DestroyWindow(sdl_window);
        SDL_Quit();
    }

    SDL_Window* Window::init_window() {
        SDL_Init(SDLFlags);

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        
        return SDL_CreateWindow(windowName.c_str(), width, height, windowFlags);
    }

    SDL_GLContext Window::init_context() {
        return SDL_GL_CreateContext(sdl_window);
    }

    void Window::swap_window() {
        assert(sdl_window != nullptr);
        SDL_GL_SwapWindow(sdl_window);
    }
}

