#pragma once

#include <vector>
#include <stdexcept>

#include <SDL3/SDL_video.h>

#include <glm/fwd.hpp>

#include "system.h"

namespace garnish {
class RenderDevice : garnish::System {
   public:
    struct InitInfo {
        void* nativeWindow;
        uint32_t width;
        uint32_t height;
        bool vsync = false;
        void* pNext;
    };
    virtual bool init(InitInfo& info) = 0;
    virtual bool draw_frame(ECSController& world) = 0;
    virtual bool set_uniform(glm::mat4 mvp) = 0;
    virtual void cleanup() = 0;

    virtual uint32_t setup_mesh(const std::vector<float>& vertices, const std::vector<uint32_t>& indices) = 0;

    enum class Primitive{
        CUBE
    };

    uint32_t setup_mesh(const std::string& mesh_path);
    uint32_t setup_mesh(const Primitive& primitive);

    virtual uint32_t load_texture(const std::string& texture_path) = 0;

    void update(ECSController& world) override = 0;

    SDL_Window* window = nullptr;
};
}  // namespace garnish
