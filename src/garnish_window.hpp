#pragma once

#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_opengl.h>


#include <cstdint>
#include <string>

namespace garnish {
    class GarnishWindow {
        public:
            GarnishWindow(uint32_t w, uint32_t h, std::string name);
            GarnishWindow(uint32_t w, uint32_t h, std::string name, uint64_t sdl_flags, uint64_t window_flags);
            ~GarnishWindow();

            GarnishWindow(const GarnishWindow&) = delete;
            GarnishWindow &operator=(const GarnishWindow&) = delete;

            void SwapWindow();

            bool shouldClose = false;

            SDL_Window *window;

        private:
            void InitWindow();
            const uint32_t width;
            const uint32_t height;
            std::string windowName;
            uint64_t SDLFlags;
            uint64_t windowFlags;

            SDL_GLContext glContext;

            // static bool resizingEventWatcher(void *data, SDL_Event *event); maybe later lol
    };
}