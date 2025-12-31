#include "garnish_app.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <thread>

#include "Physics/physics_system.hpp"
#include "Utility/camera.hpp"
#include "Utility/imgui_raii.hpp"
#include "render_device.hpp"

#ifdef _OPENGL_RENDERING
#include "ogl_renderer.hpp"
#endif
#ifdef _VULKAN_RENDERING
#include "vulkan_renderer.hpp"
#endif

namespace garnish {
App::App(const CreateInfo& createInfo)
    : width(createInfo.width),
      height(createInfo.height),
      fps(createInfo.targetFps) {
    make_render_device(createInfo);
    ecsController.set(renderDevice.get());

    ecsController.register_component<RigidBody>();
    ecsController.register_component<Transform>();
    ecsController.register_component<SphereCollider>();
    ecsController.register_component<Camera>();
    ecsController.register_component<Renderable>();
    ecsController.register_component<Material>();
    ecsController.register_component<PointLight>();

    init_imgui();
}

App::~App() noexcept {
    terminate_imgui();
    if (renderDevice) {
        renderDevice->cleanup();
    }
}

void App::init() {}

void App::run() {
    using clock = std::chrono::steady_clock;
    auto nextFrame = clock::now();
    constexpr auto MICROSECONDS_PER_SECOND = std::chrono::microseconds{1'000'000};
    auto frameTime = MICROSECONDS_PER_SECOND / fps;

    while (!shouldClose) {
        const auto frameStart = clock::now();
        if (frameStart > nextFrame + frameTime) nextFrame = frameStart;
        auto dt = frameStart - (nextFrame - frameTime);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (imguiEnabled_) {
                ImGui_ImplSDL3_ProcessEvent(&event);
            }
            switch (event.type) {
                case SDL_EVENT_QUIT: shouldClose = true; break;
                case SDL_EVENT_WINDOW_RESIZED: refresh_window_size(); break;
                default: break;
            }
        }

        for (auto& updateFunction : updateFunctions) {
            updateFunction(ecsController);
        }

        physicsSystem.update(ecsController);

        begin_imgui_frame();
        if (imguiEnabled_) {
            for (auto& callback : imguiCallbacks_) {
                callback(ecsController);
            }
        }
        end_imgui_frame();

        renderDevice->update(ecsController);

        nextFrame += frameTime;
        std::this_thread::sleep_until(nextFrame);
    }
}

bool App::handle_poll_event() {
    SDL_Event event{};
    if (!SDL_PollEvent(&event)) {
        return false;
    }
    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED || event.type == SDL_EVENT_QUIT) {
        shouldClose = true;
    }
    return true;
}

void App::handle_all_events() {
    SDL_PumpEvents();
    while (handle_poll_event()) {
    }
}

void App::init_imgui() {
    static ImGuiContextRAII imguiCtx;

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    renderDevice->init_imgui_backend();
}

void App::terminate_imgui() {
    renderDevice->shutdown_imgui_backend();
    ImGui_ImplSDL3_Shutdown();
}

void App::begin_imgui_frame() {
    if (!imguiEnabled_) return;

    renderDevice->new_imgui_frame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void App::end_imgui_frame() {
    if (!imguiEnabled_) return;
    ImGui::Render();
}

void App::make_render_device(const CreateInfo& createInfo) {
    switch (createInfo.backend) {
#ifdef _OPENGL_RENDERING
        case RenderingBackend::OpenGL:
            window = std::make_unique<SDLWindowManager>(
                SDL_INIT_VIDEO | SDL_INIT_EVENTS,
                "hello window",
                static_cast<int>(width),
                static_cast<int>(height),
                SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
            );
#ifdef __APPLE__
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

            renderDevice = std::make_unique<OpenGLRenderDevice>(RenderDevice::InitInfo{
                .nativeWindow = window->get(),
                .width = width,
                .height = height,
                .vsync = false,
                .assetPath = createInfo.assetPath
            });
            break;
#endif
#ifdef _VULKAN_RENDERING
        case RenderingBackend::Vulkan:
            window = std::make_unique<SDLWindowManager>(
                SDL_INIT_VIDEO | SDL_INIT_EVENTS,
                "hello window",
                static_cast<int>(width),
                static_cast<int>(height),
                SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
            );
            renderDevice = std::make_unique<vulkan::VulkanRenderDevice>(RenderDevice::InitInfo{
                .nativeWindow = window->get(),
                .width = width,
                .height = height,
                .vsync = false,
                .assetPath = createInfo.assetPath
            });
            break;
#endif
        default: throw std::runtime_error("no rendering device created");
    }
}

void App::refresh_window_size() {
    int w = 0;
    int h = 0;
    SDL_GetWindowSize(window->get(), &w, &h);
    width = static_cast<uint32_t>(w);
    height = static_cast<uint32_t>(h);
}
}  // namespace garnish
