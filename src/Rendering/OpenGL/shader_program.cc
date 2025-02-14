#include "shader_program.hpp"

#include <vector>
#include <stdexcept>

#include <glm/gtc/type_ptr.hpp>

#include "OpenGL.hpp"

#include "../../Utility/read_file.hpp"

namespace garnish {
    ShaderProgram::ShaderProgram(const std::string& vertexShaderPath, const std::string& fragmentShaderPath) 
        : handle(glCreateProgram()) {

        std::vector<char> vertexShaderSource = ReadFile(vertexShaderPath);
        const char* vertexSource = vertexShaderSource.data();

        std::vector<char> fragmentShaderSource = ReadFile(fragmentShaderPath);
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

        glAttachShader(handle, vertexShader);
        glAttachShader(handle, fragmentShader);
        glLinkProgram(handle);

        glGetProgramiv(handle, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(handle, 512, NULL, infoLog);
            throw std::runtime_error("Shader Program failed to link:\n" + std::string{ infoLog });
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    ShaderProgram::~ShaderProgram() {
        glDeleteProgram(handle);
    }

    void ShaderProgram::Use() {
        glUseProgram(handle);
    }

    void ShaderProgram::SetUniform(const std::string& name, const glm::mat4& mat) {
        Use();
        glUniformMatrix4fv(glGetUniformLocation(handle, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }
}