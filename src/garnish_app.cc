#include "garnish_app.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <thread>
#include <chrono>
#include "Physics/physics_system.hpp"

#ifdef _OPENGL_RENDERING
#include <imgui_impl_opengl3.h>

#include "ogl_renderer.hpp"
#endif
#ifdef _VULKAN_RENDERING
#include <SDL3/SDL_vulkan.h>

#include "vulkan_renderer.hpp"
#endif

namespace garnish {
App::App(CreateInfo createInfo)
    : width(createInfo.width),
      height(createInfo.height),
      fps(createInfo.targetFps),
      renderDevice(make_render_device(createInfo.backend)),
      window(nullptr) {
    init();
    RenderDevice::InitInfo info{}; // value-initialize
    switch (createInfo.backend) {
#ifdef _OPENGL_RENDERING
        case RenderingBackend::OpenGL:
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
            SDL_GL_SetAttribute(
                SDL_GL_CONTEXT_PROFILE_MASK,
                SDL_GL_CONTEXT_PROFILE_CORE
            );
            window.reset(init_window(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE));
            if (!window) throw std::runtime_error("Failed to create OpenGL window");
            info = {
                .nativeWindow = window.get(),
                .width = width,
                .height = height,
                .vsync = false,
                .assetPath = createInfo.assetPath
            };
            renderDevice->init(info);
            break;
#endif

#ifdef _VULKAN_RENDERING
        case RenderingBackend::Vulkan:
            window.reset(init_window(
                SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY |
                SDL_WINDOW_RESIZABLE
            ));
            if (!window) throw std::runtime_error("Failed to create Vulkan window");
            info = {
                .nativeWindow = window.get(),
                .width = width,
                .height = height,
                .vsync = false,
                .assetPath = createInfo.assetPath
            };
            renderDevice->init(info);
            break;
#endif

        default:
            throw std::runtime_error("not valid backend");
    }

    ecsController.set(renderDevice.get());

    ecsController.register_component<RigidBody>();
    ecsController.register_component<Transform>();
    ecsController.register_component<Camera>();
    ecsController.register_component<Renderable>();

    ecsController.register_system<PhysicsSystem>(0);
}

App::~App() noexcept {
    if (renderDevice) {
        renderDevice->cleanup();
    }

    SDL_Quit();
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

        ecsController.update_all();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    shouldClose = true;
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                    refresh_window_size();
                    break;
                default:
                    break;
            }
        }
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
    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED ||
        event.type == SDL_EVENT_QUIT) {
        shouldClose = true;
    }
    return true;
}

void App::handle_all_events() {
    SDL_PumpEvents();
    while (handle_poll_event()) {
    }
}

SDL_Window* App::init_window(int64_t flags) const {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    return SDL_CreateWindow("hello window", static_cast<int>(width), static_cast<int>(height), flags);
}

void App::init_imgui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

#ifdef _OPENGL_RENDERING
    ImGui_ImplSDL3_InitForOpenGL(window.get(), SDL_GL_GetCurrentContext());
    ImGui_ImplOpenGL3_Init();
#endif
}

void App::terminate_imgui() {
#ifdef _OPENGL_RENDERING
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
#endif
    ImGui::DestroyContext();
}

std::unique_ptr<RenderDevice> App::make_render_device(
    RenderingBackend backend
) {
    switch (backend) {
#ifdef _OPENGL_RENDERING
        case RenderingBackend::OpenGL:
            return std::make_unique<OpenGLRenderDevice>();
#endif
#ifdef _VULKAN_RENDERING
        case RenderingBackend::Vulkan:
            return std::make_unique<VulkanRenderDevice>();
#endif
        default:
            throw std::runtime_error("no rendering device created");
    }
}

void App::refresh_window_size() {
    int w = 0;
    int h = 0;
    SDL_GetWindowSize(window.get(), &w, &h);
    width = static_cast<uint32_t>(w);
    height = static_cast<uint32_t>(h);
}
}  // namespace garnish
