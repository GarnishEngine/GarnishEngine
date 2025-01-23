#pragma once

#include "carrot_window.hpp"
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_video.h>
#include <cstdint>

namespace Garnish {
    CarrotWindow::CarrotWindow(uint32_t w, uint32_t h, std::string name) 
        : width(w), height(h), windowName(name) {
        SDLFlags = (SDL_INIT_VIDEO | SDL_INIT_EVENTS);
        windowFlags = (SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY |
                       SDL_WINDOW_RESIZABLE);
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
        SDL_Window *window = SDL_CreateWindow(windowName, width, height, windowFlags);
    }
    }
    