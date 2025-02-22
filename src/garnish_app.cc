#include "garnish_app.hpp"

#include <stdexcept>

typedef std::chrono::duration<float> fsec;
namespace garnish {
    app::app(int32_t w, int32_t h) : WIDTH(w), HEIGHT(h), garnishWindow(w, h, "hello window") {}
    app::app() : WIDTH(800), HEIGHT(600), garnishWindow(800, 600, "hello window") {}

    void app::run() {
        InitImGui();
  
        if (glewInit() != GLEW_OK) {
          throw std::runtime_error("GLEW failed to initialize");
        }

        while (!shouldClose()) {
            handle_all_events();
            
            // ImGui Prep Frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();

            glViewport(0, 0, WIDTH, HEIGHT);
            glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            ecsManager.ExecuteSystems();
            
            // ImGui Render Frame
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            const ImGuiIO& io = ImGui::GetIO();
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                auto currentContextBackup = SDL_GL_GetCurrentContext();
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
                SDL_GL_MakeCurrent(garnishWindow.sdl_window, currentContextBackup);
            }

            garnishWindow.SwapWindow();
        }

        TerminateImGui();
    }
    
    bool app::handle_poll_event() {
        event event{};

        ImGui_ImplSDL3_ProcessEvent(&event.sdl_event);

        if (!event.state)
            return false;
        if (event.sdl_event.window.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            garnishWindow.shouldClose = true;
        }
        if (event.sdl_event.window.type == SDL_EVENT_WINDOW_RESIZED) {
            garnishWindow.pairWindowSize(&WIDTH, &HEIGHT);
            glViewport(0, 0, WIDTH, HEIGHT);
        }  

        return true;
    }
    
    void app::handle_all_events() {
        SDL_PumpEvents();

        while (handle_poll_event()) {}
    }

    void app::InitImGui() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |=
            ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |=
            ImGuiConfigFlags_NavEnableGamepad;            // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // IF using Docking Branch
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
  
        // Setup Platform/Renderer backends
        ImGui_ImplSDL3_InitForOpenGL(garnishWindow.sdl_window,
                                     garnishWindow.glContext);
        ImGui_ImplOpenGL3_Init();
    }

    void app::TerminateImGui() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }
}
