#pragma once

#include <vector>

#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

using Index = uint32_t;

struct Geometry {
    std::vector<Vertex> vertices;
    std::vector<Index> indices;
};

Geometry createUnitCubeGeometry();
// Geometry createUnitSphereGeometry(); // TODO
