#pragma once
#include <string>
#include "system.h"
#include <glm/mat4x4.hpp>

namespace garnish {
    class ShaderProgram {
        public:
            ShaderProgram(const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
            ShaderProgram(const ShaderProgram& other) = delete;
            ShaderProgram(ShaderProgram&& other) noexcept = default;
            ShaderProgram& operator=(const ShaderProgram& other) = delete;
            ShaderProgram& operator=(ShaderProgram&& other) noexcept = default;
            ~ShaderProgram();

            void use();
            void set_uniform(const std::string& name, const glm::mat4& mat);

        private:
            unsigned int handle;
    };
}