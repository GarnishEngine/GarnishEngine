#include "shader_program.hpp"

#include <GL/glew.h>

#include <array>
#include <format>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "Utility/read_file.hpp"

namespace garnish {
namespace {
constexpr GLenum get_shader_type(std::string_view shaderPath) {
    if (shaderPath.ends_with(".frag") || shaderPath.ends_with(".fragment")) {
        return GL_FRAGMENT_SHADER;
    }
    return GL_VERTEX_SHADER;
}
}  // namespace

ShaderProgram::ShaderProgram(std::string_view vertexShaderPath, std::string_view fragmentShaderPath)
    : handle(glCreateProgram()) {
    unsigned int vertexShader = compile_shader(vertexShaderPath);
    unsigned int fragmentShader = compile_shader(fragmentShaderPath);

    glAttachShader(handle, vertexShader);
    glAttachShader(handle, fragmentShader);
    glLinkProgram(handle);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void ShaderProgram::cleanup() const noexcept {
    glDeleteProgram(handle);
}

void ShaderProgram::use() const noexcept {
    glUseProgram(handle);
}

void ShaderProgram::set_uniform(const std::string& name, const glm::mat4& mat) const {
    use();
    glUniformMatrix4fv(
        glGetUniformLocation(handle, name.c_str()),
        1,
        GL_FALSE,
        glm::value_ptr(mat)
    );
}

unsigned int ShaderProgram::compile_shader(std::string_view shaderPath) {
    std::vector<char> shaderSource = read_file(shaderPath);

    const GLenum shaderType = get_shader_type(shaderPath);
    const char* source = shaderSource.data();

    unsigned int shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success = 0;
    constexpr size_t kInfoLogSize = 512;
    std::array<char, kInfoLogSize> infoLog{};
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, kInfoLogSize, nullptr, infoLog.data());
        throw std::runtime_error(
            std::format("Shader {} failed to compile:\n{}", shaderPath, infoLog.data())
        );
    }
    return shader;
}
}  // namespace garnish