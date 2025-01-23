// needs to be changed to support opengl

#include "carrot_window.hpp"
#include "sage_pipeline.hpp"
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>


namespace garnish {
    CarrotWindow::CarrotWindow(uint32_t w, uint32_t h, std::string name) 
        : width(w), height(h), windowName(name) {
        SDLFlags = (SDL_INIT_VIDEO | SDL_INIT_EVENTS);
        
        #ifdef _VULKAN_RENDERING
        windowFlags = (SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY |
                       SDL_WINDOW_RESIZABLE);
        #else
        windowFlags = (SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY |
                       SDL_WINDOW_RESIZABLE);
        #endif

        InitWindow();
    }

    CarrotWindow::CarrotWindow(uint32_t w, uint32_t h, std::string name,
                               uint64_t sdl_flags, uint64_t window_flags) 
        : width(w),
        height(h),
        windowName(name),
        SDLFlags(sdl_flags),
        windowFlags(window_flags) {
      InitWindow();
    }

    CarrotWindow::~CarrotWindow() {
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void CarrotWindow::InitWindow() {
        SDL_Init(SDLFlags);
        SDL_Window *window = SDL_CreateWindow(windowName.c_str(), width, height, windowFlags);
        // SDL_AddEventWatch(resizingEventWatcher, window); maybe later
    }


}
    