#include "Utility/sdl_raii.hpp"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>

#include <format>
#include <stdexcept>

namespace garnish {

SDLWindowManager::SDLWindowManager(
    uint32_t initFlags,
    const char* title,
    int width,
    int height,
    SDL_WindowFlags windowFlags
) {
    if (!SDL_Init(initFlags)) {
        throw std::runtime_error(std::format("SDL_Init failed: {}", SDL_GetError()));
    }

    window_.reset(SDL_CreateWindow(title, width, height, windowFlags));
    if (!window_) {
        SDL_Quit();
        throw std::runtime_error(std::format("SDL_CreateWindow failed: {}", SDL_GetError()));
    }
}

SDLWindowManager::~SDLWindowManager() noexcept {
    window_.reset();
    SDL_Quit();
}

}  // namespace garnish
