#pragma once

#include "Rendering/OpenGL/OpenGL.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>

#include <cstdint>
#include <string>

namespace garnish {
    class window {
        public:
            window(uint32_t w, uint32_t h, std::string name);
            window(uint32_t w, uint32_t h, std::string name, uint64_t sdl_flags, uint64_t window_flags);
            ~window();

            window(const window&) = delete;
            window &operator=(const window&) = delete;

            void SwapWindow();
            bool shouldClose = false;
            void pairWindowSize() {
                SDL_GetWindowSize(sdl_window, &width, &height);
            }
            void pairWindowSize(int32_t *width, int32_t *height) {
                SDL_GetWindowSize(sdl_window, width, height);
            }

            private: 
            void InitWindow();
            int32_t width;
            int32_t height;

            std::string windowName;

            uint64_t SDLFlags;
            uint64_t windowFlags;

            SDL_Window *sdl_window;

            SDL_GLContext glContext;

            // static bool resizingEventWatcher(void *data, SDL_Event *event); maybe later lol
    };
}
