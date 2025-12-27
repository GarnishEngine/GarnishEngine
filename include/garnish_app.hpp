#pragma once

#include <SDL3/SDL_video.h>
#include <ecs_controller.h>

#include <Rendering/render_device.hpp>
#include <cstdint>
#include <functional>
#include <glm/vec2.hpp>
#include <memory>

#include "Physics/physics_system.hpp"
#include "Utility/sdl_raii.hpp"

namespace garnish {
class RenderDevice;

enum class RenderingBackend : uint8_t {
#ifdef _OPENGL_RENDERING
    OpenGL,
#endif
#ifdef _VULKAN_RENDERING
    Vulkan,
#endif
};

class App {
   public:
    static constexpr uint32_t DEFAULT_WIDTH = 800;
    static constexpr uint32_t DEFAULT_HEIGHT = 600;
    static constexpr uint32_t DEFAULT_TARGET_FPS = 144;

    struct CreateInfo {
        RenderingBackend backend = RenderingBackend::OpenGL;
        uint32_t width = DEFAULT_WIDTH;
        uint32_t height = DEFAULT_HEIGHT;
        uint32_t targetFps = DEFAULT_TARGET_FPS;
        std::string assetPath = "";
    };
    App(CreateInfo createInfo = {
            .backend = RenderingBackend::OpenGL,
            .width = DEFAULT_WIDTH,
            .height = DEFAULT_HEIGHT,
            .targetFps = DEFAULT_TARGET_FPS,
            .assetPath = ""
        });
    virtual ~App() noexcept;
    App(const App&) = delete;
    App& operator=(const App&) = delete;
    App(App&&) = delete;
    App& operator=(App&&) = delete;

    virtual void run();
    virtual bool handle_poll_event();
    virtual void handle_all_events();

    [[nodiscard]] const std::unique_ptr<RenderDevice>&
    get_render_device() const noexcept {
        return renderDevice;
    }

    SDL_Window* get_window() noexcept { return window.get(); }
    [[nodiscard]] const SDL_Window* get_window() const noexcept {
        return window.get();
    }

    ECSController& get_controller() noexcept { return ecsController; }
    [[nodiscard]] const ECSController& get_controller() const noexcept {
        return ecsController;
    }

    void register_update_function(
        std::function<void(ECSController&)> updateFunction
    ) {
        updateFunctions.push_back(std::move(updateFunction));
    }

   private:
    std::unique_ptr<RenderDevice> renderDevice;
    ECSController ecsController;
    bool shouldClose = false;
    uint32_t width;
    uint32_t height;
    uint32_t fps;
    UniqueSDLWindow window;
    PhysicsSystem physicsSystem;
    std::vector<std::function<void(ECSController&)>> updateFunctions;

    virtual void init();
    void init_imgui();
    void terminate_imgui();
    [[nodiscard]] SDL_Window* init_window(int64_t flags) const;
    std::unique_ptr<RenderDevice> make_render_device(RenderingBackend backend);
    void refresh_window_size();
};
}  // namespace garnish
