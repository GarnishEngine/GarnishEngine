#include "garnish_app.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>

#include <cstddef>
#include <memory>
#include <render_device.hpp>
#include <stdexcept>
#include <thread>

#ifdef _OPENGL_RENDERING
#include <imgui_impl_opengl3.h>

#include <ogl_renderer.hpp>
#endif
#ifdef _VULKAN_RENDERING
#include <SDL3/SDL_vulkan.h>

#include <vulkan_renderer.hpp>
#endif

namespace garnish {
using namespace std::chrono_literals;
using namespace std::chrono;

App::App(CreateInfo createInfo)
    : width(createInfo.width),
      height(createInfo.height),
      fps(createInfo.targetFps),
      renderDevice(make_render_device(createInfo.backend)),
      window(nullptr) {
    init();
    RenderDevice::InitInfo info;
    switch (createInfo.backend) {
#ifdef _OPENGL_RENDERING
        case RenderingBackend::OpenGL:
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
            SDL_GL_SetAttribute(
                SDL_GL_CONTEXT_PROFILE_MASK,
                SDL_GL_CONTEXT_PROFILE_CORE
            );
            window = init_window(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
            if (!window) throw std::runtime_error("wtf");
            info = {
                .nativeWindow = window,
                .width = static_cast<uint32_t>(width),
                .height = static_cast<uint32_t>(height),
                .vsync = false
            };
            renderDevice->init(info);
            break;
#endif

#ifdef _VULKAN_RENDERING
        case RenderingBackend::Vulkan:
            window = init_window(
                SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY |
                SDL_WINDOW_RESIZABLE
            );
            if (!window) throw std::runtime_error("wtf vulkan");
            info = {
                .nativeWindow = window,
                .width = static_cast<uint32_t>(width),
                .height = static_cast<uint32_t>(height),
                .vsync = false
            };
            renderDevice->init(info);
            break;
#endif

        default:
            throw std::runtime_error("not valid backend");
    }
    ecsController.set(renderDevice.get());
    // init_imgui();
}

App::~App() {
    // terminate_imgui();
}

void App::init() {}

void App::run() {
    auto nextFrame = high_resolution_clock::now();
    auto frameTime = 1000000us / fps;

    while (!shouldClose) {
        const auto frameStart = high_resolution_clock::now();
        if (frameStart > nextFrame + frameTime) nextFrame = frameStart;
        duration<double> dt = frameStart - (nextFrame - frameTime);

        // ImGui Prep Frame
        // ImGui_ImplOpenGL3_NewFrame();
        // ImGui_ImplSDL3_NewFrame();
        // ImGui::NewFrame();

        ecsController.update_all();

        // ImGui Render Frame
        // ImGui::Render();
        // ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // const ImGuiIO& io = ImGui::GetIO();
        // if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        //     auto* currentContextBackup = SDL_GL_GetCurrentContext();
        //     ImGui::UpdatePlatformWindows();
        //     ImGui::RenderPlatformWindowsDefault();
        //     SDL_GL_MakeCurrent(window, currentContextBackup);
        // }

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    shouldClose = true;
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                    pair_window_size(&width, &height);
                    // glViewport(0, 0, width, height);
                    break;
                default:
                    break;
            }

            // ImGui_ImplSDL3_ProcessEvent(&event);
        }
        renderDevice->update(ecsController);
        // swap_window();
        nextFrame += frameTime;
        std::this_thread::sleep_until(nextFrame);
    }
    // terminate_imgui();

    SDL_Quit();
}

bool App::handle_poll_event() {
    SDL_Event* event = nullptr;
    SDL_PollEvent(event);

    // ImGui_ImplSDL3_ProcessEvent(event);
    if (!event) {
        return false;
    }
    if (event->window.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        shouldClose = true;
    }
    if (event->window.type == SDL_EVENT_WINDOW_RESIZED) {
        pair_window_size(&width, &height);
        // glViewport(0, 0, width, height);
    }
    return true;
}

void App::handle_all_events() {
    SDL_PumpEvents();
    while (handle_poll_event()) {
    }
}
SDL_Window* App::init_window(int64_t flags) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    return SDL_CreateWindow("hello window", width, height, flags);
}

void App::init_imgui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    io.ConfigFlags |=
        ImGuiConfigFlags_DockingEnable;  // IF using Docking Branch
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(window, SDL_GL_GetCurrentContext());
    ImGui_ImplOpenGL3_Init();
}

void App::terminate_imgui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

std::unique_ptr<RenderDevice> App::make_render_device(
    RenderingBackend backend
) {
    using namespace garnish;
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
            return nullptr;
    }
}
}  // namespace garnish
