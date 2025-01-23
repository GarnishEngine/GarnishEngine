#pragma once

#include "vulkan/vulkan_core.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>
#include <string>

namespace Garnish {

    class CarrotWindow {
    public:
        CarrotWindow(uint32_t w, uint32_t h, std::string name);
        CarrotWindow(uint32_t w, uint32_t h, std::string name, uint64_t sdl_flags, uint64_t window_flags);
        ~CarrotWindow();

    private: 
            void InitWindow();
            const uint32_t width;
            const uint32_t height;
            std::string windowName;
            SDL_Window *window;
            uint64_t SDLFlags;
            uint64_t windowFlags;
    };

}