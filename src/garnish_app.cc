#include "garnish_app.hpp"
#include "Utility/garnish_debug.hpp"
#include <chrono>
#include <memory>

typedef std::chrono::duration<float> fsec;
const int32_t FRAME_RATE = 60;

namespace garnish {
    bool GarnishApp::handle_poll_event() {
        GarnishEvent event{};
        if (!event.state) return false;

        if (event.event.window.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            garnishWindow.shouldClose = true;
        }

        for (const auto &entity : entities) {
             entity->update(event);
        }

        return true;
    }
    
    void GarnishApp::handle_all_events() {
        while (handle_poll_event()) {}
    }

    void GarnishApp::run() {
        if (glewInit() != GLEW_OK) {
            throw std::runtime_error("GLEW failed to initialize");
        }
        std::shared_ptr<Camera> cam = std::make_shared<Camera>();
        entities.push_back(cam);

        ShaderProgram shaderProgram{ "shaders/shader.vert", "shaders/shader.frag" };

        std::vector<float> vertices = {
            -0.5f, -0.5f, 0.0f,
             0.0f,  0.5f, 0.0f,
             0.5f, -0.5f, 0.0f
        };

        std::vector<float> colors = {
             1.0f, 0.0f, 0.0f,
             0.0f, 1.0f, 0.0f,
             0.0f, 0.0f, 1.0f,
        };

        std::vector<unsigned int> indices = {
            0, 1, 2
        };

        unsigned int VAO;
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        VertexBufferObject VBO0{ vertices };
        ElementBufferObject EBO{ indices };

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        VertexBufferObject VBO1{ colors };

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        // int frame_num = -1;
        SDL_Event event;
        tp end_time = hrclock::now();
        
        while (!shouldClose()) {
            
            tp start_time = hrclock::now();

            int32_t dt = duration_cast<ms>(start_time - end_time).count();

            if (dt > 1000/FRAME_RATE) {
                    
                handle_all_events();
                glViewport(0, 0, WIDTH, HEIGHT);
                glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);

                shaderProgram.Use();

                glm::mat4 model{1.0f};
                glm::mat4 projection = glm::perspective(
                    glm::radians(60.0f), (float)WIDTH / (float)HEIGHT, 0.01f,
                    1000.0f);

                glm::mat4 mvp = projection * cam->ViewMatrix() * model;

                shaderProgram.SetUniform("mvp", mvp);

                // glBindVertexArray(VAO); // Dont need this because we only have
                // one VAO
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                garnishWindow.SwapWindow();

                end_time = start_time;
                // TODO: make so things work with dt, probably should move it to class member or something

                // std::cerr << "frame: " << ++frame_num
                //             << " ms_elapsed_since_last: " << dt << '\n';
            }
        }

        glDeleteVertexArrays(1, &VAO);
    }
}
