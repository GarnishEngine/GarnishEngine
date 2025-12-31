#pragma once

#include <SDL3/SDL_video.h>

#include <glm/fwd.hpp>
#include <string>

#include "geometry.hpp"

namespace garnish {
class ECSController;  // forward declaration

class RenderDevice {
   public:
    RenderDevice() = default;
    explicit RenderDevice(SDL_Window* w) : window(w) {}
    virtual ~RenderDevice() = default;
    struct InitInfo {
        void* nativeWindow{};
        uint32_t width{};
        uint32_t height{};
        bool vsync{};
        void* pNext{};
        std::string assetPath;
    };

    // Lifecycle
    virtual void cleanup() = 0;

    // Core rendering
    virtual bool draw_frame(ECSController& world) = 0;
    virtual void update(ECSController& world) = 0;

    // Resource loading
    virtual uint32_t setup_mesh(const Geometry& geometry) = 0;
    uint32_t setup_mesh(const std::string& mesh_path);
    virtual uint32_t load_texture(const std::string& texture_path) = 0;

    // ImGui integration
    virtual void init_imgui_backend() {}
    virtual void shutdown_imgui_backend() {}
    virtual void new_imgui_frame() {}

    SDL_Window* window = nullptr;
};
}  // namespace garnish