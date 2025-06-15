#pragma once

#include "OpenGL.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>

#include <cstdint>
#include <string>

namespace garnish {
    class Window {
        public:
            Window(int32_t w, int32_t h, std::string name);
            Window(int32_t w, int32_t h, std::string name, uint64_t sdl_flags, uint64_t window_flags);
            ~Window();

            Window(const Window&) = delete;
            Window &operator=(const Window&) = delete;
            Window(Window&&) = delete;
            Window &operator=(Window&&) = delete;
            
            void swap_window();
            void pair_window_size() {
                SDL_GetWindowSize(sdl_window, &width, &height);
            }
            void pair_window_size(int32_t *width, int32_t *height) {
                SDL_GetWindowSize(sdl_window, width, height);
            }
            [[nodiscard]] SDL_Window* get_sdl_window() const {
                return sdl_window;
            }
            [[nodiscard]] SDL_GLContext get_context() const {
                return glContext;
            }
        private: 
            SDL_Window* init_window();
            SDL_GLContext init_context();
            int32_t width;
            int32_t height;

            uint64_t SDLFlags;
            uint64_t windowFlags;
            std::string windowName;
            
            SDL_Window* sdl_window;
            SDL_GLContext glContext;
            // static bool resizingEventWatcher(void *data, SDL_Event *event); maybe later lol
    };
}
