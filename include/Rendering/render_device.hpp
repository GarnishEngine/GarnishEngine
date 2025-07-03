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
    virtual bool draw_frame(ECSController& world) = 0;
    virtual void cleanup() = 0;
    virtual uint32_t setup_mesh(const std::string& mesh_path) = 0;
    virtual uint32_t load_texture(const std::string& texture_path) = 0;
    void update(ECSController& world) override = 0;

    SDL_Window* window = nullptr;
};
}  // namespace garnish