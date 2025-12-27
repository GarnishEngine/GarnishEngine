#pragma once

#include <SDL3/SDL_video.h>
#include <system.h>

#include <glm/fwd.hpp>
#include <string>

#include "geometry.hpp"

namespace garnish {
class ECSController;  // forward declaration

class RenderDevice : public garnish::System {
   public:
    virtual ~RenderDevice() = default;
    struct InitInfo {
        void* nativeWindow{};
        uint32_t width{};
        uint32_t height{};
        bool vsync{};
        void* pNext{};
        std::string assetPath;
    };
    virtual bool init(const InitInfo& info) = 0;
    virtual bool draw_frame(ECSController& world) = 0;
    virtual void cleanup() = 0;

    virtual uint32_t setup_mesh(const Geometry& geometry) = 0;
    uint32_t setup_mesh(const std::string& mesh_path);
    virtual uint32_t load_texture(const std::string& texture_path) = 0;

    void update(ECSController& world) override = 0;

    SDL_Window* window = nullptr;
};
}  // namespace garnish