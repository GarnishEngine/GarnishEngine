#include "ogl_renderer.hpp"

#include <SDL3/SDL_video.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#include <stb_image.h>
#include <tiny_obj_loader.h>

#include <chrono>
#include <cstddef>
#include <format>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "Utility/camera.hpp"
#include "Utility/sdl_raii.hpp"
#include "ecs_controller.h"
#include "ogl_debug.hpp"
#include "shared.hpp"

namespace garnish {
namespace {
[[nodiscard]] void* buffer_offset(std::size_t offset) noexcept {
    return reinterpret_cast<void*>(offset);  // NOLINT
}
}  // namespace

using hrclock = std::chrono::high_resolution_clock;
using tp = std::chrono::time_point<hrclock>;
using ms = std::chrono::duration<double, std::milli>;
using us = std::chrono::microseconds;

OpenGLRenderDevice::OpenGLRenderDevice(const RenderDevice::InitInfo& info)
    : RenderDevice(static_cast<SDL_Window*>(info.nativeWindow)) {
    if (auto* raw = SDL_GL_CreateContext(window)) {
        glContext.reset(raw);
        SDL_GL_MakeCurrent(window, glContext.get());
    } else {
        throw std::runtime_error(std::format("SDL_GL_CreateContext failed: {}", SDL_GetError()));
    }

    glbinding::initialize(SDL_GL_GetProcAddress);
    ogl_debug::initialize();

    gl::glViewport(
        0,
        0,
        static_cast<gl::GLsizei>(info.width),
        static_cast<gl::GLsizei>(info.height)
    );
    gl::glEnable(gl::GL_DEPTH_TEST);

    shaderProgram = std::make_unique<ShaderProgram>(
        info.assetPath + "shaders/shader.vert",
        info.assetPath + "shaders/shader.frag"
    );
}

// Public overrides - Lifecycle
void OpenGLRenderDevice::cleanup() {}

// Public overrides - Core rendering
bool OpenGLRenderDevice::draw_frame(ECSController& world) {
    // Acquire camera
    glm::mat4 view{1.0F};
    glm::vec3 cameraPos{0.0F, 0.0F, 5.0F};
    auto cameras = world.get_entities<garnish::Camera>();
    if (!cameras.empty()) {
        auto& cam = world.get_component<garnish::Camera>(cameras[0]);
        view = cam.view_matrix();
        cameraPos = cam.position;
    }

    // Setup projection
    int w = 0;
    int h = 0;
    SDL_GetWindowSize(window, &w, &h);
    if (h == 0) h = 1;
    constexpr float kFovDeg = 60.0F;
    constexpr float kNear = 0.01F;
    constexpr float kFar = 1000.0F;
    glm::mat4 proj = glm::perspective(
        glm::radians(kFovDeg),
        static_cast<float>(w) / static_cast<float>(h),
        kNear,
        kFar
    );

    // Acquire light (first PointLight entity or defaults)
    glm::vec3 lightPos{0.0F, 5.0F, 5.0F};
    glm::vec3 lightColor{1.0F, 1.0F, 1.0F};
    auto lights = world.get_entities<PointLight>();
    if (!lights.empty()) {
        auto& light = world.get_component<PointLight>(lights[0]);
        lightPos = light.position;
        lightColor = light.color * light.intensity;
    }

    gl::glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);

    shaderProgram->use();

    // Set global uniforms
    shaderProgram->set_uniform("view", view);
    shaderProgram->set_uniform("projection", proj);
    shaderProgram->set_uniform("viewPos", cameraPos);
    shaderProgram->set_uniform("lightPos", lightPos);
    shaderProgram->set_uniform("lightColor", lightColor);
    shaderProgram->set_uniform("diffuseTexture", 0);

    // Default material values
    constexpr glm::vec3 kDefaultAmbient{0.1F, 0.1F, 0.1F};
    constexpr glm::vec3 kDefaultDiffuse{1.0F, 1.0F, 1.0F};
    constexpr glm::vec3 kDefaultSpecular{0.5F, 0.5F, 0.5F};
    constexpr float kDefaultShininess = 32.0F;

    auto entities = world.get_entities<Renderable>();
    for (Entity entity : entities) {
        auto& dra = world.get_component<Renderable>(entity);

        // Build model matrix
        glm::mat4 model{1.0F};
        if (world.has_component<Transform>(entity)) {
            auto& tf = world.get_component<Transform>(entity);
            model = glm::translate(model, tf.position) * glm::toMat4(tf.rotation);
        }
        shaderProgram->set_uniform("model", model);

        // Set material uniforms
        if (world.has_component<Material>(entity)) {
            auto& mat = world.get_component<Material>(entity);
            shaderProgram->set_uniform("material_ambient", mat.ambient);
            shaderProgram->set_uniform("material_diffuse", mat.diffuse);
            shaderProgram->set_uniform("material_specular", mat.specular);
            shaderProgram->set_uniform("material_shininess", mat.shininess);
        } else {
            shaderProgram->set_uniform("material_ambient", kDefaultAmbient);
            shaderProgram->set_uniform("material_diffuse", kDefaultDiffuse);
            shaderProgram->set_uniform("material_specular", kDefaultSpecular);
            shaderProgram->set_uniform("material_shininess", kDefaultShininess);
        }

        gl::glActiveTexture(gl::GL_TEXTURE0);
        gl::glBindTexture(gl::GL_TEXTURE_2D, textures[dra.texHandle].id);
        gl::glBindVertexArray(meshes[dra.meshHandle].VAO);
        gl::glDrawElements(
            gl::GL_TRIANGLES,
            meshes[dra.meshHandle].size,
            gl::GL_UNSIGNED_INT,
            nullptr
        );
        gl::glBindTexture(gl::GL_TEXTURE_2D, 0);
        gl::glBindVertexArray(0);
    }

    render_imgui();

    SDL_GL_SwapWindow(window);
    return true;
}

void OpenGLRenderDevice::render_imgui() {
    ImDrawData* drawData = ImGui::GetDrawData();
    if (drawData) {
        ImGui_ImplOpenGL3_RenderDrawData(drawData);
    }
}

void OpenGLRenderDevice::init_imgui_backend() {
    ImGui_ImplSDL3_InitForOpenGL(window, SDL_GL_GetCurrentContext());
    ImGui_ImplOpenGL3_Init();
}

void OpenGLRenderDevice::shutdown_imgui_backend() {
    ImGui_ImplOpenGL3_Shutdown();
}

void OpenGLRenderDevice::new_imgui_frame() {
    ImGui_ImplOpenGL3_NewFrame();
}

void OpenGLRenderDevice::update(ECSController& world) {
    draw_frame(world);
}

// Public overrides - Resource loading
uint32_t OpenGLRenderDevice::setup_mesh(const Geometry& geometry) {
    OGLMesh mesh{};
    gl::glGenVertexArrays(1, &mesh.VAO);
    gl::glGenBuffers(1, &mesh.VBO);
    gl::glGenBuffers(1, &mesh.EBO);

    gl::glBindVertexArray(mesh.VAO);
    gl::glBindBuffer(gl::GL_ARRAY_BUFFER, mesh.VBO);

    gl::glBufferData(
        gl::GL_ARRAY_BUFFER,
        static_cast<gl::GLsizeiptr>(geometry.vertices.size() * sizeof(Vertex)),
        geometry.vertices.data(),
        gl::GL_STATIC_DRAW
    );

    gl::glBindBuffer(gl::GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    gl::glBufferData(
        gl::GL_ELEMENT_ARRAY_BUFFER,
        static_cast<gl::GLsizeiptr>(geometry.indices.size() * sizeof(unsigned int)),
        geometry.indices.data(),
        gl::GL_STATIC_DRAW
    );

    // vertex3d positions
    gl::glEnableVertexAttribArray(0);
    gl::glVertexAttribPointer(
        0,
        3,
        gl::GL_FLOAT,
        gl::GL_FALSE,
        sizeof(Vertex),
        buffer_offset(offsetof(Vertex, position))
    );
    gl::glEnableVertexAttribArray(1);
    gl::glVertexAttribPointer(
        1,
        3,
        gl::GL_FLOAT,
        gl::GL_FALSE,
        sizeof(Vertex),
        buffer_offset(offsetof(Vertex, normal))
    );
    // vertex3d texture coords
    gl::glEnableVertexAttribArray(2);
    gl::glVertexAttribPointer(
        2,
        2,
        gl::GL_FLOAT,
        gl::GL_FALSE,
        sizeof(Vertex),
        buffer_offset(offsetof(Vertex, uv))
    );
    gl::glBindVertexArray(0);

    mesh.size = static_cast<gl::GLsizei>(geometry.indices.size());

    meshes.push_back(std::move(mesh));
    return meshes.size() - 1;
}

uint32_t OpenGLRenderDevice::load_texture(const std::string& texture_path) {
    int mTexWidth = 0;
    int mTexHeight = 0;
    int nrChannels = 0;
    gl::GLuint texID = 0;
    UniqueSTBImage textureData(
        stbi_load(texture_path.c_str(), &mTexWidth, &mTexHeight, &nrChannels, 0)
    );

    if (!textureData) {
        throw std::runtime_error("texture load failed");
    }

    gl::glGenTextures(1, &texID);
    gl::glBindTexture(gl::GL_TEXTURE_2D, texID);
    gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MIN_FILTER, gl::GL_LINEAR_MIPMAP_LINEAR);
    gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_S, gl::GL_REPEAT);
    gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_T, gl::GL_REPEAT);
    gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MAG_FILTER, gl::GL_LINEAR);

    gl::glTexImage2D(
        gl::GL_TEXTURE_2D,
        0,
        gl::GL_RGB,
        mTexWidth,
        mTexHeight,
        0,
        gl::GL_RGB,
        gl::GL_UNSIGNED_BYTE,
        textureData.get()
    );

    gl::glPixelStorei(gl::GL_UNPACK_ALIGNMENT, 1);
    gl::glGenerateMipmap(gl::GL_TEXTURE_2D);
    gl::glBindTexture(gl::GL_TEXTURE_2D, 0);

    textures.emplace_back(texID);
    return textures.size() - 1;
}

}  // namespace garnish
