#include "garnish_mesh.hpp"
#include "OpenGL/OpenGL.hpp"
#include "OpenGL/gl_buffer.hpp"
#include "garnish_texture.hpp"
#include <cstdint>
#include "stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace garnish {
    void GarnishMesh::setupMesh() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex),
                    &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                    indices.size() * sizeof(unsigned int), &indices[0],
                    GL_STATIC_DRAW);

        // vertex positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex),
                                (void *)0);
        // vertex normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex),
                                (void *)offsetof(vertex, color));
        // vertex texture coords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex),
                                (void *)offsetof(vertex, texCoord));

        glBindVertexArray(0);
    }

    GarnishMesh::GarnishMesh(std::vector<vertex> vertices,
                             std::vector<uint32_t> indices,
                             std::vector<texture> textures)
        : vertices(vertices), indices(indices), textures(textures) {
    }

    GarnishMesh::GarnishMesh(std::vector<vertex> vertices, std::vector<u_int32_t> indices) : vertices(vertices), indices(indices) {
    }

    void GarnishMesh::loadModel(std::string modelPath) {
        indices.clear();
        vertices.clear();
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                                modelPath.c_str())) {
            throw std::runtime_error(warn + err);
        }

        for (const auto &shape : shapes) {
            for (const auto &index : shape.mesh.indices) {
                vertex vert{};
                vert.pos = {attrib.vertices[3 * index.vertex_index + 0],
                                attrib.vertices[3 * index.vertex_index + 1],
                                attrib.vertices[3 * index.vertex_index + 2]};

                vert.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

                vert.color = {1.0f, 1.0f, 1.0f};

                vertices.push_back(vert);
            

                indices.push_back(indices.size());
            }
        }
    }
    void GarnishMesh::loadTexture(std::string texturePath) {
        gTexture.loadTexture(texturePath);
        glBindTexture(GL_TEXTURE_2D, gTexture.texture);
        glBindVertexArray(VAO);
    }

    void GarnishMesh::draw() {
        glBindVertexArray(VAO); // Dont need this because we only have
        // one VAO
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }


    void GarnishMesh::deleteVertexArray() {
        glDeleteVertexArrays(1, &VAO);
    }
}