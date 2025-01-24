#include <string>

namespace garnish {
    class ShaderProgram {
        public:
            ShaderProgram(const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
            ShaderProgram(const ShaderProgram& other) = delete;
            ShaderProgram(ShaderProgram&& other) noexcept = default;
            ShaderProgram& operator=(const ShaderProgram& other) = delete;
            ShaderProgram& operator=(ShaderProgram&& other) noexcept = default;
            ~ShaderProgram();

            void Use();

        private:
            unsigned int handle;
    };
}