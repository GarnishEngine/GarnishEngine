#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <vector>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

[[nodiscard]] constexpr bool operator==(
    const Vertex& vertex,
    const Vertex& other
) noexcept {
    return vertex.position == other.position &&
           vertex.normal == other.normal && vertex.uv == other.uv;
}

using Index = uint32_t;

struct Geometry {
    std::vector<Vertex> vertices;
    std::vector<Index> indices;
};

Geometry createUnitCubeGeometry();
// Geometry createUnitSphereGeometry(); // TODO

namespace std {
template <>
struct hash<Vertex> {
    [[nodiscard]] constexpr size_t operator()(Vertex const& vertex
    ) const noexcept {
        return ((hash<glm::vec3>()(vertex.position) ^
                 (hash<glm::vec3>()(vertex.normal) << 1)) >>
                1) ^
               (hash<glm::vec2>()(vertex.uv) << 1);
    }
};
}  // namespace std
