#include "garnish_mesh.hpp"
#include "OpenGL.hpp"
#include "garnish_texture.hpp"
#include <cstdint>
#include "stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace garnish {
    GLsizeiptr gl_size(std::size_t n) noexcept {
        return static_cast<GLsizeiptr>(n);
    }
    void Mesh::setup_mesh() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        glBufferData(GL_ARRAY_BUFFER, gl_size(vertices.size()) * sizeof(vertex3d),
                    vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                    gl_size(indices.size()) * sizeof(unsigned int), indices.data(),
                    GL_STATIC_DRAW);

        // vertex3d positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex3d),
                                nullptr);
        // vertex3d normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex3d),
                                (void *)offsetof(vertex3d, color));
        // vertex3d texture coords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex3d),
                                (void *)offsetof(vertex3d, texCoord));

        glBindVertexArray(0);
    }

    Mesh::Mesh(std::vector<vertex3d> vertices,
               std::vector<uint32_t> indices,
               std::vector<base_texture> textures)
        : vertices(vertices), indices(indices), textures(textures) {
    }

    Mesh::Mesh(std::vector<vertex3d> vertices, std::vector<u_int32_t> indices) : vertices(vertices), indices(indices) {
    }

    void Mesh::load_mesh(const std::string& mesh_path) {
        indices.clear();
        vertices.clear();
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                                mesh_path.c_str())) {
            throw std::runtime_error(warn + err);
        }

        for (const auto &shape : shapes) {
            for (const auto &index : shape.mesh.indices) {
                vertex3d vert{};
                vert.pos = {attrib.vertices[3 * index.vertex_index + 0],
                                attrib.vertices[3 * index.vertex_index + 1],
                                attrib.vertices[3 * index.vertex_index + 2]};

                vert.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0F - attrib.texcoords[2 * index.texcoord_index + 1]};

                vert.color = {1.0F, 1.0F, 1.0F};

                vertices.push_back(vert);
            

                indices.push_back(indices.size());
            }
        }
    }
    void Mesh::load_texture(const std::string& texture_path) {
        gTexture.load_texture(texture_path);
        glBindTexture(GL_TEXTURE_2D, gTexture.texture);
        glBindVertexArray(VAO);
    }
    void Mesh::load_texture(const g_texture &texture) {
        glBindTexture(GL_TEXTURE_2D, texture.texture);
        glBindVertexArray(VAO);
    }

    void Mesh::draw() {
        glBindVertexArray(VAO); // Dont need this because we only have
        // one VAO
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void Mesh::delete_vertex_array() {
        glDeleteVertexArrays(1, &VAO);
    }
}