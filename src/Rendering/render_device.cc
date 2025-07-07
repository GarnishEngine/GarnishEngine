#include "render_device.hpp"

#include <tiny_obj_loader.h>

namespace garnish {
    uint32_t RenderDevice::setup_mesh(const std::string& mesh_path) {
        std::vector<float> vertices;
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
                // Positions
                vertices.push_back(attrib.vertices[3 * index.vertex_index + 0]);
                vertices.push_back(attrib.vertices[3 * index.vertex_index + 1]);
                vertices.push_back(attrib.vertices[3 * index.vertex_index + 2]);

                // Texture coords
                vertices.push_back(attrib.texcoords[2 * index.texcoord_index + 0]);
                vertices.push_back(1.0F - attrib.texcoords[2 * index.texcoord_index + 1]);

                // Stand in color
                vertices.push_back(1.0f);
                vertices.push_back(1.0f);
                vertices.push_back(1.0f);

                indices.push_back(indices.size());
            }
        }

        return setup_mesh(vertices, indices);
    }

    uint32_t RenderDevice::setup_mesh(const Primitive& primitive) {
        // TODO switch statement

        return setup_mesh(std::vector<float>{ 0.0f, 0.0f, 0.0f }, std::vector<uint32_t>{ 0 });
    }
}
