#pragma once

#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>

#include <cstdint>
#include <memory>
#include <vector>

#include "Utility/sdl_raii.hpp"
#include "render_device.hpp"
#include "shader_program.hpp"
namespace garnish {
class OpenGLRenderDevice : public RenderDevice {
   public:
    explicit OpenGLRenderDevice(const RenderDevice::InitInfo& info);
    OpenGLRenderDevice(const OpenGLRenderDevice&) = delete;
    OpenGLRenderDevice& operator=(const OpenGLRenderDevice&) = delete;
    OpenGLRenderDevice(OpenGLRenderDevice&&) = delete;
    OpenGLRenderDevice& operator=(OpenGLRenderDevice&&) = delete;
    ~OpenGLRenderDevice() override { cleanup(); }

    // Lifecycle
    void cleanup() override;

    // Core rendering
    bool draw_frame(ECSController& world) override;
    void update(ECSController& world) override;

    // Resource loading
    using RenderDevice::setup_mesh;
    uint32_t setup_mesh(const Geometry& geometry) override;
    uint32_t load_texture(const std::string& texture_path) override;
    void set_shader();

    // ImGui integration
    void init_imgui_backend() override;
    void shutdown_imgui_backend() override;
    void new_imgui_frame() override;

   private:
    struct OGLMesh {
        gl::GLuint VAO = 0;
        gl::GLuint VBO = 0;
        gl::GLuint EBO = 0;
        gl::GLsizei size = 0;

        OGLMesh() = default;
        OGLMesh(const OGLMesh&) = delete;
        OGLMesh& operator=(const OGLMesh&) = delete;
        OGLMesh(OGLMesh&& other) noexcept
            : VAO(other.VAO),
              VBO(other.VBO),
              EBO(other.EBO),
              size(other.size) {
            other.VAO = other.VBO = other.EBO = 0;
            other.size = 0;
        }
        OGLMesh& operator=(OGLMesh&& other) noexcept {
            if (this != &other) {
                this->~OGLMesh();
                VAO = other.VAO;
                VBO = other.VBO;
                EBO = other.EBO;
                size = other.size;
                other.VAO = other.VBO = other.EBO = 0;
                other.size = 0;
            }
            return *this;
        }
        ~OGLMesh() {
            if (EBO) gl::glDeleteBuffers(1, &EBO);
            if (VBO) gl::glDeleteBuffers(1, &VBO);
            if (VAO) gl::glDeleteVertexArrays(1, &VAO);
        }
    };

    struct OGLTexture {
        gl::GLuint id = 0;
        OGLTexture() = default;
        explicit OGLTexture(gl::GLuint tex)
            : id(tex) {}
        OGLTexture(const OGLTexture&) = delete;
        OGLTexture& operator=(const OGLTexture&) = delete;
        OGLTexture(OGLTexture&& other) noexcept
            : id(other.id) {
            other.id = 0;
        }
        OGLTexture& operator=(OGLTexture&& other) noexcept {
            if (this != &other) {
                if (id) gl::glDeleteTextures(1, &id);
                id = other.id;
                other.id = 0;
            }
            return *this;
        }
        ~OGLTexture() {
            if (id) gl::glDeleteTextures(1, &id);
        }
    };

    void render_imgui();

    std::unique_ptr<ShaderProgram> shaderProgram;
    std::vector<OGLTexture> textures;
    std::vector<OGLMesh> meshes;
    GLContext glContext;
};

}  // namespace garnish