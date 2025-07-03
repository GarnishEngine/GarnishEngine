#pragma once
#include <stb_image.h>
#include <tiny_obj_loader.h>

#include <OpenGL.hpp>
#include <cstdint>
#include <garnish_app.hpp>
#include <render_device.hpp>
#include <string>
#include <vector>

#include "SDL3/SDL_video.h"
#include "framebuffer.hpp"
#include "garnish_texture.hpp"
#include "shader_program.hpp"

namespace garnish {

using OGLTexture = uint32_t;
struct OGLMesh {
    uint32_t VAO;
    uint32_t VBO;
    uint32_t EBO;
    uint32_t size;
};
class OpenGLRenderDevice : public RenderDevice {
   public:
    OpenGLRenderDevice() = default;
    OpenGLRenderDevice(const OpenGLRenderDevice&) = delete;
    OpenGLRenderDevice& operator=(const OpenGLRenderDevice&) = delete;
    OpenGLRenderDevice(OpenGLRenderDevice&&) = delete;
    OpenGLRenderDevice& operator=(OpenGLRenderDevice&&) = delete;
    ~OpenGLRenderDevice() = default;
    bool init(InitInfo& info) override;
    void set_size(unsigned int width, unsigned int height) override;
    bool draw() override;
    void cleanup() override;
    void update(ECSController& world) override;

    // uint64_t get_flags() override;
    void set_shader();
    // mesh & texture helpers (from ogl_renderer.cc)

    uint32_t setup_mesh(const std::string& mesh_path);
    void delete_mesh(Mesh mesh);
    uint32_t load_texture(const std::string& texture_path);

   private:
    // ShaderProgram mShader;
    // Framebuffer mFramebuffer;
    // int mTriangleCount = 0;
    std::vector<OGLTexture> textures;
    std::vector<OGLMesh> meshes;

    SDL_GLContext glContext = nullptr;
};

}  // namespace garnish