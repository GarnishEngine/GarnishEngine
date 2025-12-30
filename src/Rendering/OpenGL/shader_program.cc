#include "shader_program.hpp"

#include <GL/glew.h>

#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <vector>

#include "Utility/read_file.hpp"

namespace garnish {
ShaderProgram::ShaderProgram(
    const std::string& vertexShaderPath,
    const std::string& fragmentShaderPath
)
    : handle(glCreateProgram()) {
    unsigned int vertexShader = compile_shader(vertexShaderPath);
    unsigned int fragmentShader = compile_shader(fragmentShaderPath);

    glAttachShader(handle, vertexShader);
    glAttachShader(handle, fragmentShader);
    glLinkProgram(handle);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

ShaderProgram::~ShaderProgram() {
    cleanup();
}

void ShaderProgram::cleanup() const {
    glDeleteProgram(handle);
}

void ShaderProgram::use() const {
    glUseProgram(handle);
}

void ShaderProgram::set_uniform(
    const std::string& name,
    const glm::mat4& mat
) const {
    use();
    glUniformMatrix4fv(
        glGetUniformLocation(handle, name.c_str()),
        1,
        GL_FALSE,
        glm::value_ptr(mat)
    );
}

unsigned int ShaderProgram::compile_shader(const std::string& shaderPath) {
    std::vector<char> shaderSource = read_file(shaderPath);

    const char* source = shaderSource.data();

    unsigned int shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success = 0;
    constexpr size_t kInfoLogSize = 512;
    std::array<char, kInfoLogSize> infoLog{};
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, kInfoLogSize, nullptr, infoLog.data());
        throw std::runtime_error(
            "Shader " + shaderPath + " failed to compile:\n" +
            std::string{infoLog.data()}
        );
    }
    return shader;
}
}  // namespace garnish