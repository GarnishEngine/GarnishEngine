#pragma once
#include <shader_program.hpp>
#include <shared.hpp>
#include <vector>
#include <Utility/camera.hpp>
#include <Physics/physics_system.hpp>

#define GLEW_STATIC
#define GL_PTR_OFFSET(i) reinterpret_cast<void*>(static_cast<intptr_t>(i))

#include <GL/glew.h>
#include <SDL3/SDL_opengl.h>

#include <cstdint>
#include <memory>

#include "render_device.hpp"
#include "Utility/sdl_raii.hpp"

namespace garnish {

class OpenGLRenderDevice : public RenderDevice {
   public:
    OpenGLRenderDevice() = default;
    OpenGLRenderDevice(const OpenGLRenderDevice&) = delete;
    OpenGLRenderDevice& operator=(const OpenGLRenderDevice&) = delete;
    OpenGLRenderDevice(OpenGLRenderDevice&&) = delete;
    OpenGLRenderDevice& operator=(OpenGLRenderDevice&&) = delete;
    ~OpenGLRenderDevice() override = default;
    bool init(InitInfo& info) override;
    bool draw_frame(ECSController& world) override;
    void cleanup() override;
    void update(ECSController& world) override;
    void set_shader();

    uint32_t setup_mesh(const std::string& mesh_path) override;
    uint32_t load_texture(const std::string& texture_path) override;

   private:
    struct OGLVertex3d {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec2 texCoord;
        OGLVertex3d() = default;
    };

    struct OGLMesh {
        GLuint VAO = 0;
        GLuint VBO = 0;
        GLuint EBO = 0;
        GLsizei size = 0;

        OGLMesh() = default;
        OGLMesh(const OGLMesh&) = delete;
        OGLMesh& operator=(const OGLMesh&) = delete;
        OGLMesh(OGLMesh&& other) noexcept
            : VAO(other.VAO), VBO(other.VBO), EBO(other.EBO), size(other.size) {
            other.VAO = other.VBO = other.EBO = 0;
            other.size = 0;
        }        
        OGLMesh& operator=(OGLMesh&& other) noexcept {
            if (this != &other) {
                this->~OGLMesh();
                VAO = other.VAO; VBO = other.VBO; EBO = other.EBO; size = other.size;
                other.VAO = other.VBO = other.EBO = 0; other.size = 0;
            }
            return *this;
        }
        ~OGLMesh() {
            if (EBO) glDeleteBuffers(1, &EBO);
            if (VBO) glDeleteBuffers(1, &VBO);
            if (VAO) glDeleteVertexArrays(1, &VAO);
        }
    };

    struct OGLTexture {
        GLuint id = 0;
        OGLTexture() = default;
        explicit OGLTexture(GLuint tex) : id(tex) {}
        OGLTexture(const OGLTexture&) = delete;
        OGLTexture& operator=(const OGLTexture&) = delete;
        OGLTexture(OGLTexture&& other) noexcept : id(other.id) { other.id = 0; }
        OGLTexture& operator=(OGLTexture&& other) noexcept {
            if (this != &other) {
                if (id) glDeleteTextures(1, &id);
                id = other.id; other.id = 0;
            }
            return *this;
        }
        ~OGLTexture() {
            if (id) glDeleteTextures(1, &id);
        }
    };

    std::unique_ptr<ShaderProgram> shaderProgram;
    std::vector<OGLTexture> textures;
    std::vector<OGLMesh> meshes;
    GLContext glContext; // RAII wrapper
};

}  // namespace garnish