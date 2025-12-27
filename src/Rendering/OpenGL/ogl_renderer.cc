#include "ogl_renderer.hpp"

#include <SDL3/SDL_video.h>
#include <ecs_controller.h>
#include <stb_image.h>
#include <tiny_obj_loader.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <cstddef>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>


namespace garnish {
namespace { 
void* buffer_offset(std::size_t offset) {
    return reinterpret_cast<void*>(offset); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast,performance-no-int-to-ptr)
}
}

using hrclock = std::chrono::high_resolution_clock;
using tp = std::chrono::time_point<hrclock>;
using ms = std::chrono::duration<double, std::milli>;
using us = std::chrono::microseconds;

bool OpenGLRenderDevice::init(const InitInfo& info) {
    window = static_cast<SDL_Window*>(info.nativeWindow);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_PROFILE_MASK,
        SDL_GL_CONTEXT_PROFILE_CORE
    );

    auto* raw = SDL_GL_CreateContext(window); // was 'auto raw'
    if (!raw) {
        std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError();
        return false;
    }
    glContext.reset(raw);

    SDL_GL_MakeCurrent(window, glContext.get());

    if (glewInit() != GLEW_OK) {
        throw std::runtime_error("GLEW failed to initialize");
    }

    glViewport(0, 0, static_cast<GLsizei>(info.width), static_cast<GLsizei>(info.height));
    glEnable(GL_DEPTH_TEST);

    shaderProgram = std::make_unique<ShaderProgram>(
        info.assetPath + "shaders/shader.vert",
        info.assetPath + "shaders/shader.frag"
    );
    return true;
}

bool OpenGLRenderDevice::draw_frame(ECSController& world) {
    // Acquire camera (first one if multiple)
    glm::mat4 view{1.0F};
    glm::mat4 proj{1.0F};
    auto cameras = world.get_entities<garnish::Camera>();
    if (!cameras.empty()) {
        auto &cam = world.get_component<garnish::Camera>(cameras[0]);
        view = cam.view_matrix();
    }
    int w = 0;
    int h = 0;
    SDL_GetWindowSize(window, &w, &h);
    if (h == 0) h = 1;
    constexpr float kFovDeg = 60.0F;
    constexpr float kNear = 0.01F;
    constexpr float kFar = 1000.0F;
    proj = glm::perspective(
        glm::radians(kFovDeg),
        static_cast<float>(w) / static_cast<float>(h),
        kNear,
        kFar
    );

    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shaderProgram->use();

    auto entities = world.get_entities<Renderable>();
    for (Entity entity : entities) {
        auto &dra = world.get_component<Renderable>(entity);
        glm::mat4 model{1.0F};
        if (world.has_component<Transform>(entity)) {
            auto &tf = world.get_component<Transform>(entity);
            model = glm::translate(model, tf.position) * glm::toMat4(tf.rotation);
        }
        glm::mat4 mvp = proj * view * model;
        shaderProgram->set_uniform("mvp", mvp);

        glBindTexture(GL_TEXTURE_2D, textures[dra.texHandle].id);
        glBindVertexArray(meshes[dra.meshHandle].VAO);
        glDrawElements(GL_TRIANGLES, meshes[dra.meshHandle].size, GL_UNSIGNED_INT, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
    }
    SDL_GL_SwapWindow(window);
    return true;
}

void OpenGLRenderDevice::update(ECSController& world) {
    draw_frame(world);
}

void OpenGLRenderDevice::cleanup() {}


uint32_t OpenGLRenderDevice::setup_mesh(const Geometry& geometry) {
    OGLMesh mesh{};
    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);

    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(geometry.vertices.size() * sizeof(Vertex)),
        geometry.vertices.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(geometry.indices.size() * sizeof(unsigned int)),
        geometry.indices.data(),
        GL_STATIC_DRAW
    );

    // vertex3d positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        buffer_offset(offsetof(Vertex, position))
    );
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        buffer_offset(offsetof(Vertex, normal))
    );
    // vertex3d texture coords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        buffer_offset(offsetof(Vertex, uv)) // removed pointer arithmetic
    );
    glBindVertexArray(0);

    mesh.size = static_cast<GLsizei>(geometry.indices.size());

    meshes.push_back(std::move(mesh));
    return meshes.size() - 1;
}

uint32_t OpenGLRenderDevice::load_texture(const std::string& texture_path) {
    int mTexWidth = 0;
    int mTexHeight = 0; 
    int nrChannels = 0;
    unsigned int texID = -1;
    // stbi_set_flip_vertically_on_load(true);
    unsigned char* textureData = stbi_load(
        texture_path.c_str(),
        &mTexWidth,
        &mTexHeight,
        &nrChannels,
        0
    );

    if (!textureData) {
        stbi_image_free(textureData);
        throw std::runtime_error("texture load failed");
    }

    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR_MIPMAP_LINEAR
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB,
        mTexWidth,
        mTexHeight,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        textureData
    );

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(textureData);

    textures.emplace_back(texID);
    return textures.size() - 1;
}

}  // namespace garnish
