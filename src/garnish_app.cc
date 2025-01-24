#include "garnish_app.hpp"

#include "Utility/read_file.hpp"

namespace garnish {
    void GarnishApp::run() {
        if (glewInit() != GLEW_OK) {
            throw std::runtime_error("GLEW failed to initialize");
        }

        std::vector<char> vertexShaderSource = ReadFile("shaders/shader.vert");
        const char* vertexSource = vertexShaderSource.data();

        std::vector<char> fragmentShaderSource = ReadFile("shaders/shader.frag");
        const char* fragmentSource = fragmentShaderSource.data();

        unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexSource, NULL);
        glCompileShader(vertexShader);

        int success;
        char infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            throw std::runtime_error("Vertex Shader failed to compile:\n" + std::string{ infoLog });
        }

        unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
        glCompileShader(fragmentShader);

        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            throw std::runtime_error("Fragment Shader failed to compile:\n" + std::string{ infoLog });
        }

        unsigned int shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
            throw std::runtime_error("Shader Program failed to link:\n" + std::string{ infoLog });
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        std::vector<float> vertices = {
            -0.5f, -0.5f, 0.0f,
             0.0f,  0.5f, 0.0f,
             0.5f, -0.5f, 0.0f
        };

        std::vector<unsigned int> indices = {
            0, 1, 2
        };

        unsigned int VBO, VAO, EBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), (void*)vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), (void*)indices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        SDL_Event event;
        while (!shouldClose()) {
            glViewport(0, 0, WIDTH, HEIGHT);
            glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glUseProgram(shaderProgram);

            // glBindVertexArray(VAO); // Dont need this because we only have one VAO
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            garnishWindow.SwapWindow();

            SDL_PollEvent(&event);
            if (event.window.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                garnishWindow.shouldClose = true;
            }
        }

        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteProgram(shaderProgram);
    }
}
