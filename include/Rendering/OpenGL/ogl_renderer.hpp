#pragma once
#include <shader_program.hpp>
#include <shared.hpp>
#include <vector>

#define GLEW_STATIC
#define GL_PTR_OFFSET(i) reinterpret_cast<void*>(static_cast<intptr_t>(i))

#include <GL/glew.h>
#include <SDL3/SDL_opengl.h>

#include <cstdint>

#include "render_device.hpp"

namespace garnish {

class OpenGLRenderDevice : public RenderDevice {
   public:
    OpenGLRenderDevice() = default;
    OpenGLRenderDevice(const OpenGLRenderDevice&) = delete;
    OpenGLRenderDevice& operator=(const OpenGLRenderDevice&) = delete;
    OpenGLRenderDevice(OpenGLRenderDevice&&) = delete;
    OpenGLRenderDevice& operator=(OpenGLRenderDevice&&) = delete;
    ~OpenGLRenderDevice() = default;
    bool init(InitInfo& info) override;
    bool draw_frame(ECSController& world) override;
    void cleanup() override;
    void update(ECSController& world) override;
    bool set_uniform(glm::mat4 mvp) override;
    void set_shader();

    uint32_t setup_mesh(const std::string& mesh_path) override;
    // void delete_mesh(uint32_t meshHandle);
    uint32_t load_texture(const std::string& texture_path) override;

   private:
    struct OGLVertex3d {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec2 texCoord;
        OGLVertex3d() = default;
    };
    struct OGLMesh {
        uint32_t VAO;
        uint32_t VBO;
        uint32_t EBO;
        uint32_t size;
    };
    using OGLTexture = uint32_t;

    std::unique_ptr<ShaderProgram> shaderProgram;
    // Framebuffer mFramebuffer;
    // int mTriangleCount = 0;
    std::vector<OGLTexture> textures;
    std::vector<OGLMesh> meshes;
    SDL_GLContext glContext = nullptr;
};

}  // namespace garnish