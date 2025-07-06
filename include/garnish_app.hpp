#pragma once
#include <SDL3/SDL_video.h>

#include <memory>

#include "ecs_controller.h"
#include "render_device.hpp"

namespace garnish {
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
    struct CreateInfo {
        RenderingBackend backend;
        uint32_t width;
        uint32_t height;
        uint32_t targetFps = 144;
    };
    App(CreateInfo createInfo = {
            .backend = RenderingBackend::OpenGL,
            .width = 800,
            .height = 600,
            .targetFps = 144
        });
    virtual ~App();
    App(const App&) = delete;
    App& operator=(const App&) = delete;
    App(App&&) = delete;
    App& operator=(App&&) = delete;
    void pair_window_size(int32_t* width, int32_t* height) {
        // SDL_GetWindowSize(window, width, height);
    }
    virtual void run();
    virtual bool handle_poll_event();
    virtual void handle_all_events();
    std::unique_ptr<RenderDevice>& get_render_device() { return renderDevice; }

    SDL_Window*& get_window();
    ECSController& get_controller() { return ecsController; }

   private:
    std::unique_ptr<RenderDevice> renderDevice;
    ECSController ecsController;
    bool shouldClose = false;
    int32_t width;
    int32_t height;
    uint32_t fps;
    SDL_Window* window;
    virtual void init();
    void init_imgui();
    void terminate_imgui();
    SDL_Window* init_window(int64_t flags);
    std::unique_ptr<RenderDevice> make_render_device(RenderingBackend backend);
    //  Window garnishWindow;
};
}  // namespace garnish
