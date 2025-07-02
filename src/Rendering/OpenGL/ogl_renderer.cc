#include "ogl_renderer.hpp"

#include <cstdint>

#include "OpenGL.hpp"
#include "SDL3/SDL_video.h"
#include "ecs_common.h"
#include "garnish_app.hpp"
#include "garnish_mesh.hpp"
#include "garnish_texture.hpp"
#include "shader_program.hpp"

namespace garnish {
void draw(int VAO, int size, int texture) {
    glBindVertexArray(VAO);  // Dont need this because we only have
    // one VAO
    glBindTexture(GL_TEXTURE_2D, texture);
    glDrawElements(GL_TRIANGLES, size, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

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

    set_size(info.width, info.height);

    // SDL_GL_SetSwapInterval(info.vsync ? 1 : 0); maybe one day

    glEnable(GL_DEPTH_TEST);
    return true;
}

void OpenGLRenderDevice::set_size(unsigned int width, unsigned int height) {
    glViewport(0, 0, width, height);
}

bool OpenGLRenderDevice::draw() {
    // glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    // glClearDepth(1.0f);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // glEnable(GL_CULL_FACE);
    // glEnable(GL_DEPTH_TEST);

    // mShader.use();
    // glBindTexture(GL_TEXTURE_2D, TexID);
    // glBindVertexArray(VAO);

    // glDrawElements(GL_TRIANGLES, indicies, GL_UNSIGNED_INT, nullptr);
    // glBindTexture(GL_TEXTURE_2D, 0);
    // glBindVertexArray(0);
    return true;
}

void OpenGLRenderDevice::update(ECSController& world) {
    for (Entity& entity : world.get_entities<drawable, texture>()) {
        auto dra = world.get_component<drawable>(entity);
        auto tex = world.get_component<texture>(entity);

        glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // mShader.use();
        glBindTexture(GL_TEXTURE_2D, tex.id);

        glBindVertexArray(dra.VAO);

        glDrawElements(GL_TRIANGLES, dra.size, GL_UNSIGNED_INT, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
    }
}
void OpenGLRenderDevice::cleanup() {}
// uint64_t OpenGLRenderDevice::get_flags() {
//     return SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
// }

Mesh OpenGLRenderDevice::setup_mesh(const std::string& mesh_path) {
    auto rawmesh = load_mesh(mesh_path);
    return setup_mesh(rawmesh.vertices, rawmesh.indices);
}

Mesh OpenGLRenderDevice::setup_mesh(
    const std::vector<OGLVertex3d>& vertices,
    const std::vector<uint32_t>& indices
) {
    Mesh mesh{};
    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);
    mesh.numIndicies = indices.size();
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
    return mesh;
}

void OpenGLRenderDevice::delete_mesh(Mesh mesh) {
    glDeleteVertexArrays(1, &mesh.VAO);
    glDeleteBuffers(1, &mesh.VBO);
    glDeleteBuffers(1, &mesh.EBO);
}

rawmesh OpenGLRenderDevice::load_mesh(const std::string& mesh_path) {
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
    return rawmesh{.vertices = vertices, .indices = indices};
}
texture OpenGLRenderDevice::load_texture(const std::string& texture_path) {
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
        return texture{texID};
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

    return texture{texID};
}

}  // namespace garnish