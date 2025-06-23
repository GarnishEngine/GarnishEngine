#pragma once
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>

#include <chrono>
#include <glm/ext/matrix_clip_space.hpp>
#include <memory>
#include <stdexcept>

#include "camera.hpp"
#include "ecs_controller.h"
#include "garnish_mesh.hpp"
#include "garnish_sprite.hpp"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"
#include "render_device.hpp"
#include "shader_program.hpp"

namespace garnish {
enum class RenderingBackend : uint8_t { OpenGL, Vulkan };
using hrclock = std::chrono::high_resolution_clock;
using tp = std::chrono::time_point<hrclock>;
using ms = std::chrono::duration<double, std::milli>;
using us = std::chrono::microseconds;

using std::chrono::duration_cast;
class App {
   public:
    struct CreateInfo {
        RenderingBackend backend;  
        uint32_t width;
        uint32_t height;
        uint32_t targetFps = 144;
    };
    App(CreateInfo createInfo = {
            .backend=RenderingBackend::OpenGL,
            .width=800,
            .height=600,
            .targetFps=144
    });
    virtual ~App();
    App(const App&) = delete;
    App& operator=(const App&) = delete;
    App(App&&) = delete;
    App& operator=(App&&) = delete;
    void pair_window_size(int32_t* width, int32_t* height) {
        SDL_GetWindowSize(window, width, height);
    }
    virtual void run();
    virtual bool handle_poll_event();
    virtual void handle_all_events();
    void swap_window();
    std::unique_ptr<RenderDevice>& getRenderDevice() { return renderDevice; }

    SDL_Window*& get_window();
    ECSController& get_controller() { return ecsController; }

   private:
    std::unique_ptr<RenderDevice> renderDevice;
    SDL_Window* window;
    ECSController ecsController;
    bool shouldClose = false;
    int32_t width;
    int32_t height;
    uint32_t fps;
    virtual void init();
    void init_imgui();
    void terminate_imgui();
    SDL_Window* init_window(int64_t flags);
    std::unique_ptr<RenderDevice> makeRenderDevice(RenderingBackend backend);
    //  Window garnishWindow;
};
}  // namespace garnish
