#include "garnish_app.hpp"

#include <stdexcept>

#include "Rendering/garnish_mesh.hpp"
#include "Rendering/garnish_sprite.hpp"
#include "Rendering/garnish_texture.hpp"

typedef std::chrono::duration<float> fsec;
namespace garnish {
    app::app(int32_t w, int32_t h) : WIDTH(w), HEIGHT(h), garnishWindow(w, h, "hello window") { Init(); }
    app::app() : WIDTH(800), HEIGHT(600), garnishWindow(800, 600, "hello window") { Init(); }

    void app::Init() {

        InitImGui();
  
        if (glewInit() != GLEW_OK) {
          throw std::runtime_error("GLEW failed to initialize");
        }

        shaderProgram = std::make_unique<ShaderProgram>("shaders/shader.vert", "shaders/shader.frag");

        // Naive Rendering system
        ecsManager.RegisterComponent<mesh>();
        ecsManager.AddSystem([](ECSManager* ecs){
            for (auto& ent : ecs->GetEntities<mesh>()) {
                auto m = ecs->GetComponent<mesh>(ent);
                m.draw();
            }
        });

        ecsManager.RegisterComponent<sprite>();
        ecsManager.AddSystem([&](ECSManager* ecs){ // TODO remove capture
            auto cam_ent = ecs->GetEntities<Camera>()[0]; // TODO this is really janky, need to do something about the camera
            auto cam = ecs->GetComponent<Camera>(cam_ent);

            shaderProgram->Use();

            glm::mat4 model{1.0f};
            model = glm::translate(model, glm::vec3{0.0f, -0.3f, 3.0f});

            model = glm::rotate(model, glm::radians(-90.0f),
                                glm::vec3{1.0f, 0.0f, 0.0f});
            model = glm::rotate(model, glm::radians(-135.0f),
                                glm::vec3{0.0f, 0.0f, 1.0f});

            glm::mat4 projection =
                glm::perspective(glm::radians(60.0f),
                                (float)WIDTH / (float)HEIGHT, 0.01f, 1000.0f);

            glm::mat4 mvp = projection * cam.ViewMatrix() * model;

            shaderProgram->SetUniform("mvp", mvp);

            for (auto& ent : ecs->GetEntities<sprite>()) {
                auto s = ecs->GetComponent<sprite>(ent);
                s.draw();
            }
        });
    }

    void app::run() {
        while (!shouldClose()) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                switch (event.type) {
                case SDL_EVENT_QUIT:
                    garnishWindow.shouldClose = true;
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                    garnishWindow.pairWindowSize(&WIDTH, &HEIGHT);
                    glViewport(0, 0, WIDTH, HEIGHT);
                    break;
                }

                ImGui_ImplSDL3_ProcessEvent(&event);
            }
            
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

        SDL_Quit();
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

        auto cam_ent = ecsManager.GetEntities<Camera>()[0]; // TODO this is really janky, need to do something about the camera
        auto cam = ecsManager.GetComponent<Camera>(cam_ent);
        // cam->update(event);

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
