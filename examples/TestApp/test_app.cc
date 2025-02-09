#include "Rendering/OpenGL/shader_program.hpp"
#include "Rendering/garnish_mesh.hpp"
#include "Rendering/garnish_texture.hpp"
#include "Utility/camera.hpp"
#include "garnish_app.hpp"
#include "Utility/log.hpp"
#include "Rendering/garnish_sprite.hpp"

const int32_t FRAME_RATE = 90;

typedef std::chrono::high_resolution_clock hrclock;
typedef std::chrono::time_point<hrclock> tp;
typedef std::chrono::milliseconds ms;
using std::chrono::duration_cast;

namespace garnish {
    class TestApp : public app { 
        public:
        bool handle_poll_event() override {
            event event{};

            if (event.sdl_event.type== SDL_EVENT_MOUSE_BUTTON_DOWN) {
                std::cout << "hi" << std::endl;
            }

            ImGui_ImplSDL3_ProcessEvent(&event.sdl_event); 
            ImGuiIO& io = ImGui::GetIO();

            
            
            if (!event.state)
                return false;

            if (event.sdl_event.window.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                garnishWindow.shouldClose = true;
            }
            if (event.sdl_event.window.type == SDL_EVENT_WINDOW_RESIZED) {
                garnishWindow.pairWindowSize(&WIDTH, &HEIGHT);
                glViewport(0, 0, WIDTH, HEIGHT);
            }  
    
            for (const auto &entity : entities) {
                entity->update(event);
            }
    
            return true;
        }
        
        void run() override {

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch



            // Setup Platform/Renderer backends
            ImGui_ImplSDL3_InitForOpenGL(garnishWindow.sdl_window, garnishWindow.glContext);
            ImGui_ImplOpenGL3_Init();

            if (glewInit() != GLEW_OK) {
                throw std::runtime_error("GLEW failed to initialize");
            }
            std::shared_ptr<Camera> cam = std::make_shared<Camera>();
            entities.push_back(cam);

            ShaderProgram shaderProgram{"shaders/shader.vert",
                                        "shaders/shader.frag"};

            // mesh gMesh;
            garnish_texture texture;
            texture.loadTexture("Textures/viking_room.png");

            // gMesh.loadMesh("Models/viking_room.obj");
            // gMesh.setupMesh();
            // gMesh.loadTexture(texture);
            sprite gSprite;
            gSprite.setupSprite();
            gSprite.loadTexture(texture);


            sprite testsprite;
            testsprite.draw();


            SDL_Event event;

            tp end_time = hrclock::now();

            while (!shouldClose()) {
                tp start_time = hrclock::now();

                float dt = duration_cast<us>(start_time - end_time).count();

                if (dt > 1000000.0F / FRAME_RATE) {

                    handle_all_events();
                    ImGui_ImplSDL3_NewFrame();
                    ImGui_ImplOpenGL3_NewFrame();
                    ImGui::NewFrame();
                    ImGui::ShowDemoWindow();
                    {
                        static float f = 0.0f;
                        static int counter = 0;
            
                        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.
            
                        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
                    
                        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                            counter++;
                        ImGui::SameLine();
                        ImGui::Text("counter = %d", counter);
            
                        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
                        ImGui::End();
                    }

                    ImGui::Render();


                    glViewport(0, 0, WIDTH, HEIGHT);
                    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

                    cam->update();
                    shaderProgram.Use();

                    glm::mat4 model{1.0f};
                    model = glm::translate(model, glm::vec3{ 0.0f, -0.3f, 3.0f });

                    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3{ 1.0f, 0.0f, 0.0f });
                    model = glm::rotate(model, glm::radians(-135.0f), glm::vec3{ 0.0f, 0.0f, 1.0f });

                    glm::mat4 projection = glm::perspective(
                        glm::radians(60.0f), (float)WIDTH / (float)HEIGHT, 0.01f,
                        1000.0f);

                    glm::mat4 mvp = projection * cam->ViewMatrix() * model;

                    shaderProgram.SetUniform("mvp", mvp);

                    
                    // gMesh.draw();
                    gSprite.draw();


                    garnishWindow.SwapWindow();
                    end_time = start_time;
                    // TODO: make so things work with dt, probably should move it to
                    // class member or something

                    // std::cerr << "frame: " << ++frame_num
                    //             << " ms_elapsed_since_last: " << dt << '\n';
                }
            }
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext();
        }
    };
}

int main() {
    garnish::TestApp app{ };
    garnish::log << "hi";
    app.run();
}
