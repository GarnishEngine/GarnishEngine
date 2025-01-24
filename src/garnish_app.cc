#include "garnish_app.hpp"

#include "Utility/OpenGL/shader_program.hpp"
#include "Utility/OpenGL/gl_buffer.hpp"
#include "Utility/camera.hpp"

namespace garnish {
    void GarnishApp::run() {
        if (glewInit() != GLEW_OK) {
            throw std::runtime_error("GLEW failed to initialize");
        }

        Camera cam{ };

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


        SDL_Event event;
        while (!shouldClose()) {
            glViewport(0, 0, WIDTH, HEIGHT);
            glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            shaderProgram.Use();

            // glBindVertexArray(VAO); // Dont need this because we only have one VAO
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            garnishWindow.SwapWindow();

            SDL_PollEvent(&event);
            if (event.window.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                garnishWindow.shouldClose = true;
            }
        }

        glDeleteVertexArrays(1, &VAO);
    }
}
