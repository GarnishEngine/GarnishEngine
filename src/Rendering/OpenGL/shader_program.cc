#include "shader_program.hpp"

#include <glbinding/gl/gl.h>

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
constexpr gl::GLenum get_shader_type(std::string_view shaderPath) {
    if (shaderPath.ends_with(".frag") || shaderPath.ends_with(".fragment")) {
        return gl::GL_FRAGMENT_SHADER;
    }
    return gl::GL_VERTEX_SHADER;
}
}  // namespace

ShaderProgram::ShaderProgram(std::string_view vertexShaderPath, std::string_view fragmentShaderPath)
    : handle(gl::glCreateProgram()) {
    gl::GLuint vertexShader = compile_shader(vertexShaderPath);
    gl::GLuint fragmentShader = compile_shader(fragmentShaderPath);

    gl::glAttachShader(handle, vertexShader);
    gl::glAttachShader(handle, fragmentShader);
    gl::glLinkProgram(handle);

    gl::glDeleteShader(vertexShader);
    gl::glDeleteShader(fragmentShader);
}

void ShaderProgram::cleanup() const noexcept {
    gl::glDeleteProgram(handle);
}

void ShaderProgram::use() const noexcept {
    gl::glUseProgram(handle);
}

void ShaderProgram::set_uniform(const std::string& name, const glm::mat4& mat) const {
    use();
    gl::glUniformMatrix4fv(
        gl::glGetUniformLocation(handle, name.c_str()),
        1,
        false,
        glm::value_ptr(mat)
    );
}

gl::GLuint ShaderProgram::compile_shader(std::string_view shaderPath) {
    std::vector<char> shaderSource = read_file(shaderPath);

    const gl::GLenum shaderType = get_shader_type(shaderPath);
    const char* source = shaderSource.data();

    gl::GLuint shader = gl::glCreateShader(shaderType);
    gl::glShaderSource(shader, 1, &source, nullptr);
    gl::glCompileShader(shader);

    gl::GLint success = 0;
    constexpr size_t kInfoLogSize = 512;
    std::array<char, kInfoLogSize> infoLog{};
    gl::glGetShaderiv(shader, gl::GL_COMPILE_STATUS, &success);
    if (success == 0) {
        gl::glGetShaderInfoLog(shader, kInfoLogSize, nullptr, infoLog.data());
        throw std::runtime_error(
            std::format("Shader {} failed to compile:\n{}", shaderPath, infoLog.data())
        );
    }
    return shader;
}
}  // namespace garnish