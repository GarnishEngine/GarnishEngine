#pragma once
#include <glm/mat4x4.hpp>
#include <string>

namespace garnish {
class ShaderProgram {
   public:
    ShaderProgram(
        const std::string& vertexShaderPath,
        const std::string& fragmentShaderPath
    );
    ShaderProgram(const ShaderProgram& other) = delete;
    ShaderProgram(ShaderProgram&& other) noexcept = default;
    ShaderProgram& operator=(const ShaderProgram& other) = delete;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept = default;
    ~ShaderProgram();

    void use() const;
    void set_uniform(const std::string& name, const glm::mat4& mat) const;
    void cleanup() const;

   private:
    [[nodiscard]] static unsigned int compile_shader(
        const std::string& shaderPath
    );

    unsigned int handle;
};
}  // namespace garnish