#pragma once

#include <glm/glm.hpp>
#include <vector>


struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

bool operator==(const Vertex& vertex, const Vertex& other);

using Index = uint32_t;

struct Geometry {
    std::vector<Vertex> vertices;
    std::vector<Index> indices;
};

Geometry createUnitCubeGeometry();
// Geometry createUnitSphereGeometry(); // TODO
