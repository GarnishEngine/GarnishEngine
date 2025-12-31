#include "render_device.hpp"

#include <tiny_obj_loader.h>

#include <format>
#include <stdexcept>

namespace garnish {
uint32_t RenderDevice::setup_mesh(const std::string& mesh_path) {
    std::vector<Vertex> vertices;
    std::vector<Index> indices;
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, mesh_path.c_str())) {
        throw std::runtime_error(std::format("{}: {}", warn, err));
    }

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vert{};
            vert.position = {
                attrib.vertices[(3 * index.vertex_index) + 0],
                attrib.vertices[(3 * index.vertex_index) + 1],
                attrib.vertices[(3 * index.vertex_index) + 2]
            };

            vert.normal = {
                attrib.normals[(3 * index.normal_index) + 0],
                attrib.normals[(3 * index.normal_index) + 1],
                attrib.normals[(3 * index.normal_index) + 2]
            };

            vert.uv = {
                attrib.texcoords[(2 * index.texcoord_index) + 0],
                1.0F - attrib.texcoords[(2 * index.texcoord_index) + 1]
            };

            vertices.push_back(vert);
            indices.push_back(indices.size());
        }
    }

    return setup_mesh(Geometry{.vertices = vertices, .indices = indices});
}
}  // namespace garnish
