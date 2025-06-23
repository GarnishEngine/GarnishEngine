#include "ogl_renderer.hpp"

#include <cstdint>

#include "OpenGL.hpp"
#include "ecs_common.h"
#include "garnish_mesh.hpp"
#include "garnish_texture.hpp"
#include "stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace garnish {

void draw(int VAO, int size, int texture) {
    glBindVertexArray(VAO);  // Dont need this because we only have
    // one VAO
    glBindTexture(GL_TEXTURE_2D, texture);
    glDrawElements(GL_TRIANGLES, size, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void delete_mesh(Mesh mesh) {
    glDeleteVertexArrays(1, &mesh.VAO);
    glDeleteBuffers(1, &mesh.VBO);
    glDeleteBuffers(1, &mesh.EBO);
}

bool OpenGLRenderDevice::init(InitInfo& info) {
    if (glewInit() != GLEW_OK) {
        throw std::runtime_error("GLEW failed to initialize");
    }
    window = static_cast<SDL_Window*>(info.nativeWindow);
    SDL_GL_MakeCurrent(window, glContext);

    if (!glContext) {
        std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError();
        return false;
    }

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
    }
}

uint64_t OpenGLRenderDevice::get_flags() {
    return SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
}

void setup_mesh(
    std::vector<OGLVertex3d> vertices,
    std::vector<uint32_t> indices,
    std::vector<BaseTexture> textures
) {
    Mesh mesh{};
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
        nullptr
    );

    // vertex3d texture coords
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(OGLVertex3d),
        (void*)offsetof(OGLVertex3d, texCoord)
    );

    glBindVertexArray(0);
}

void load_mesh(const std::string& mesh_path) {
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

            vertices.push_back(vert);

            indices.push_back(indices.size());
        }
    }
}
int load_texture(const std::string& texture_path) {
    int mTexWidth, mTexHeight, nrChannels;
    unsigned int texture;
    // stbi_set_flip_vertically_on_load(true);
    unsigned char* textureData = stbi_load(
        texture_path.c_str(),
        &mTexWidth,
        &mTexHeight,
        &nrChannels,
        0
    );

    if (textureData) {
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
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        stbi_image_free(textureData);
        return false;
    }

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR_MIPMAP_LINEAR
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        mTexWidth,
        mTexHeight,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        textureData
    );
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(textureData);

    return texture;
}

}  // namespace garnish