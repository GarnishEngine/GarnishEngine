#pragma once

#include <glbinding/gl/gl.h>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <string>
#include <string_view>

namespace garnish {
class ShaderProgram {
   public:
    ShaderProgram(std::string_view vertexShaderPath, std::string_view fragmentShaderPath);
    ShaderProgram(const ShaderProgram& other) = delete;
    ShaderProgram(ShaderProgram&& other) noexcept = default;
    ShaderProgram& operator=(const ShaderProgram& other) = delete;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept = default;
    ~ShaderProgram() { cleanup(); }

    void cleanup() const noexcept;
    void use() const noexcept;
    void set_uniform(const std::string& name, const glm::mat4& mat) const;
    void set_uniform(const std::string& name, const glm::vec3& vec) const;
    void set_uniform(const std::string& name, float value) const;
    void set_uniform(const std::string& name, int value) const;

   private:
    [[nodiscard]] static gl::GLuint compile_shader(std::string_view shaderPath);

    gl::GLuint handle;
};
}  // namespace garnish