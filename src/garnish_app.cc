#include "garnish_app.hpp"
#include <thread>


namespace garnish {
    using namespace std::chrono_literals; 
    App::App(int32_t w, int32_t h) : WIDTH(w), HEIGHT(h), garnishWindow(w, h, "hello window") { init(); }
    App::App() : WIDTH(800), HEIGHT(600), garnishWindow(800, 600, "hello window") { init(); }
    App::~App() {  terminate_imgui(); }

    void App::init() {

        init_imgui();
  
        if (glewInit() != GLEW_OK) {
          throw std::runtime_error("GLEW failed to initialize");
        }


        // Naive Rendering system
        ecsController.register_component<Mesh>();
        // ecsController.add_system([](ECSController* ecs){
        //     for (auto& ent : ecs->GetEntities<mesh>()) {
        //         auto m = ecs->GetComponent<mesh>(ent);
        //         m.draw();
        //     }
        // });

        ecsController.register_component<Sprite>();

    }

    void App::run() {
         while (!shouldClose) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                switch (event.type) {
                    case SDL_EVENT_QUIT:
                        shouldClose = true;
                        break;
                    case SDL_EVENT_WINDOW_RESIZED:
                        garnishWindow.pair_window_size(&WIDTH, &HEIGHT);
                        glViewport(0, 0, WIDTH, HEIGHT);
                        break;
                    default: 
                        break;
                }

                ImGui_ImplSDL3_ProcessEvent(&event);
            }
            
            // ImGui Prep Frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();

            glViewport(0, 0, WIDTH, HEIGHT);
            glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            ecsController.update_all();
            
            // ImGui Render Frame
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            const ImGuiIO& io = ImGui::GetIO();
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                auto *currentContextBackup = SDL_GL_GetCurrentContext();
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
                SDL_GL_MakeCurrent(garnishWindow.get_sdl_window(), currentContextBackup);
            }

            garnishWindow.swap_window();
            std::this_thread::sleep_for(8ms); // most certainly the cleanest way to do this
        }
        terminate_imgui();

        SDL_Quit();
    }
    
    bool App::handle_poll_event() {
        Event event{};

        ImGui_ImplSDL3_ProcessEvent(&event.sdl_event);

        if (!event.state) {
            return false;
        }
        if (event.sdl_event.window.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
           shouldClose = true;
        }
        if (event.sdl_event.window.type == SDL_EVENT_WINDOW_RESIZED) {
            garnishWindow.pair_window_size(&WIDTH, &HEIGHT);
            glViewport(0, 0, WIDTH, HEIGHT);
        }

        auto cam_ent = ecsController.get_entities<Camera>()[0]; // TODO this is really janky, need to do something about the camera
        auto cam = ecsController.get_component<Camera>(cam_ent);
        // cam->update(event);

        return true;
    }
    
    void App::handle_all_events() {
        SDL_PumpEvents();

        while (handle_poll_event()) {}
    }

    void App::init_imgui() {
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
        ImGui_ImplSDL3_InitForOpenGL(garnishWindow.get_sdl_window(),
                                     garnishWindow.get_context());
        ImGui_ImplOpenGL3_Init();
    }

    void App::terminate_imgui() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }
}
