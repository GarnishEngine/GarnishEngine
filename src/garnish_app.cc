#include "garnish_app.hpp"
#include "Utility/garnish_debug.hpp"
#include <memory>

namespace garnish {
    void GarnishApp::handle_poll_event() {
        GarnishEvent event{};
        if (!event.state) return;
        if (event.event.window.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
          garnishWindow.shouldClose = true;
        }
        for (const auto &entity : entities) {

          entity->update(event);
        }
    }



    void GarnishApp::run() {
        if (glewInit() != GLEW_OK) {
            throw std::runtime_error("GLEW failed to initialize");
        }
        std::shared_ptr<Camera> cam = std::make_shared<Camera>(0.1f);
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
        int i = 0;
        SDL_Event event;
        while (!shouldClose()) {
            handle_poll_event();

            glViewport(0, 0, WIDTH, HEIGHT);
            glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            shaderProgram.Use();

            glm::mat4 model{ 1.0f };
            glm::mat4 projection = glm::perspective(glm::radians(60.0f), (float)WIDTH / (float)HEIGHT, 0.01f, 1000.0f);

            glm::mat4 mvp = projection * cam->ViewMatrix() * model;

            shaderProgram.SetUniform("mvp", mvp);

            // glBindVertexArray(VAO); // Dont need this because we only have one VAO
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            garnishWindow.SwapWindow();
            // std::cout << "frame: " << i << std::endl;
            // ++i;

            
        }

        glDeleteVertexArrays(1, &VAO);
    }
}
