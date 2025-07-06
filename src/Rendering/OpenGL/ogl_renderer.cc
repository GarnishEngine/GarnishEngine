#include "ogl_renderer.hpp"

#include <SDL3/SDL_video.h>
#include <ecs_controller.h>
#include <stb_image.h>
#include <tiny_obj_loader.h>

#include <iostream>
#include <stdexcept>

namespace garnish {
using hrclock = std::chrono::high_resolution_clock;
using tp = std::chrono::time_point<hrclock>;
using ms = std::chrono::duration<double, std::milli>;
using us = std::chrono::microseconds;

bool OpenGLRenderDevice::init(InitInfo& info) {
    window = static_cast<SDL_Window*>(info.nativeWindow);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_PROFILE_MASK,
        SDL_GL_CONTEXT_PROFILE_CORE
    );
    glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError();
        return false;
    }

    if (glewInit() != GLEW_OK) {
        throw std::runtime_error("GLEW failed to initialize");
    }

    glViewport(0, 0, info.width, info.height);
    // SDL_GL_SetSwapInterval(info.vsync ? 1 : 0); maybe one day

    glEnable(GL_DEPTH_TEST);
    return true;
}

bool OpenGLRenderDevice::draw_frame(ECSController& world) {
    for (Entity& entity : world.get_entities<Renderable>()) {
        auto dra = world.get_component<Renderable>(entity);

        glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // mShader.use();
        glBindTexture(GL_TEXTURE_2D, textures[dra.texHandle]);

        glBindVertexArray(meshes[dra.meshHandle].VAO);

        glDrawElements(
            GL_TRIANGLES,
            meshes[dra.meshHandle].size,
            GL_UNSIGNED_INT,
            nullptr
        );
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

uint32_t OpenGLRenderDevice::setup_mesh(const std::string& mesh_path) {
    std::vector<OGLVertex3d> vertices;
    std::vector<uint32_t> indices;
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(
            &attrib,
            &shapes,
            &materials,
            &warn,
            &err,
            mesh_path.c_str()
        )) {
        throw std::runtime_error(warn + err);
    }

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            OGLVertex3d vert{};
            vert.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vert.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0F - attrib.texcoords[2 * index.texcoord_index + 1]
            };
            vert.color = {1.0F, 1.0F, 1.0F};
            vertices.push_back(vert);

            indices.push_back(indices.size());
        }
    }

    OGLMesh mesh{};
    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);

    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(OGLVertex3d),
        vertices.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.data(),
        GL_STATIC_DRAW
    );

    // vertex3d positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(OGLVertex3d),
        (void*)offsetof(OGLVertex3d, pos)
    );
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(OGLVertex3d),
        (void*)offsetof(OGLVertex3d, color)
    );
    // vertex3d texture coords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(OGLVertex3d),
        (void*)offsetof(OGLVertex3d, texCoord)
    );
    glBindVertexArray(0);

    mesh.size = indices.size();

    meshes.push_back(mesh);
    return meshes.size() - 1;
}

uint32_t OpenGLRenderDevice::load_texture(const std::string& texture_path) {
    int mTexWidth, mTexHeight, nrChannels = 0;
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

    textures.push_back(texID);
    return textures.size() - 1;
}

}  // namespace garnish