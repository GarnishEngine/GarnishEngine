#include "Rendering/OpenGL/shader_program.hpp"
#include "Rendering/garnish_mesh.hpp"
#include "Utility/camera.hpp"
#include "garnish_app.hpp"

const int32_t FRAME_RATE = 60;

typedef std::chrono::high_resolution_clock hrclock;
typedef std::chrono::time_point<hrclock> tp;
typedef std::chrono::milliseconds ms;
using std::chrono::duration_cast;

namespace garnish {
    class TestApp : public GarnishApp { 
        public:
        void run() override {
            if (glewInit() != GLEW_OK) {
                throw std::runtime_error("GLEW failed to initialize");
            }
            std::shared_ptr<Camera> cam = std::make_shared<Camera>();
            entities.push_back(cam);

            ShaderProgram shaderProgram{"shaders/shader.vert",
                                        "shaders/shader.frag"};

            GarnishMesh gMesh;
            gMesh.loadModel("Models/viking_room.obj");
            gMesh.setupMesh();
            gMesh.loadTexture("Textures/viking_room.png");

            SDL_Event event;

            tp end_time = hrclock::now();

            while (!shouldClose()) {
                tp start_time = hrclock::now();

                int32_t dt = duration_cast<ms>(start_time - end_time).count();

                if (dt > 1000 / FRAME_RATE) {

                    handle_all_events();
                    glViewport(0, 0, WIDTH, HEIGHT);
                    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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

                    gMesh.draw();
                    garnishWindow.SwapWindow();

                    end_time = start_time;
                    // TODO: make so things work with dt, probably should move it to
                    // class member or something

                    // std::cerr << "frame: " << ++frame_num
                    //             << " ms_elapsed_since_last: " << dt << '\n';
                }
            }
        }
    };
}

int main() {
    garnish::TestApp app{ };

    app.run();
}
