#pragma once

#include <system.h>

#include "SDL3/SDL_video.h"

namespace garnish {
class RenderDevice : garnish::System {
   public:
    struct InitInfo {
        void* nativeWindow;
        uint32_t width;
        uint32_t height;
        bool vsync = false;
    };
    virtual bool init(InitInfo& info) = 0;
    virtual void set_size(unsigned int width, unsigned int height) = 0;
    virtual bool draw() = 0;
    virtual void cleanup() = 0;
    // virtual uint64_t get_flags() = 0;
    void update(ECSController& world) override = 0;
    SDL_Window* window;
};
}  // namespace garnish