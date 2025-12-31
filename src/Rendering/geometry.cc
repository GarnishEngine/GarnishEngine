#include "geometry.hpp"

#include <numbers>
#include <ranges>


Geometry createUnitCubeGeometry() {
    Geometry geo;
    geo.vertices = {
        Vertex{
            .position = glm::vec3{-0.5f, -0.5f, -0.5f},
            .normal = glm::vec3{0.0f, 0.0f, -1.0f},
            .uv = glm::vec2{0.0f, 0.0f}
        },
        Vertex{
            .position = glm::vec3{0.5f, -0.5f, -0.5f},
            .normal = glm::vec3{0.0f, 0.0f, -1.0f},
            .uv = glm::vec2{1.0f, 0.0f}
        },
        Vertex{
            .position = glm::vec3{0.5f, 0.5f, -0.5f},
            .normal = glm::vec3{0.0f, 0.0f, -1.0f},
            .uv = glm::vec2{1.0f, 1.0f}
        },
        Vertex{
            .position = glm::vec3{-0.5f, 0.5f, -0.5f},
            .normal = glm::vec3{0.0f, 0.0f, -1.0f},
            .uv = glm::vec2{0.0f, 1.0f}
        },

        Vertex{
            .position = glm::vec3{-0.5f, -0.5f, 0.5f},
            .normal = glm::vec3{0.0f, 0.0f, 1.0f},
            .uv = glm::vec2{0.0f, 0.0f}
        },
        Vertex{
            .position = glm::vec3{0.5f, -0.5f, 0.5f},
            .normal = glm::vec3{0.0f, 0.0f, 1.0f},
            .uv = glm::vec2{1.0f, 0.0f}
        },
        Vertex{
            .position = glm::vec3{0.5f, 0.5f, 0.5f},
            .normal = glm::vec3{0.0f, 0.0f, 1.0f},
            .uv = glm::vec2{1.0f, 1.0f}
        },
        Vertex{
            .position = glm::vec3{-0.5f, 0.5f, 0.5f},
            .normal = glm::vec3{0.0f, 0.0f, 1.0f},
            .uv = glm::vec2{0.0f, 1.0f}
        },

        Vertex{
            .position = glm::vec3{-0.5f, 0.5f, 0.5f},
            .normal = glm::vec3{-1.0f, 0.0f, 0.0f},
            .uv = glm::vec2{1.0f, 0.0f}
        },
        Vertex{
            .position = glm::vec3{-0.5f, 0.5f, -0.5f},
            .normal = glm::vec3{-1.0f, 0.0f, 0.0f},
            .uv = glm::vec2{1.0f, 1.0f}
        },
        Vertex{
            .position = glm::vec3{-0.5f, -0.5f, -0.5f},
            .normal = glm::vec3{-1.0f, 0.0f, 0.0f},
            .uv = glm::vec2{0.0f, 1.0f}
        },
        Vertex{
            .position = glm::vec3{-0.5f, -0.5f, 0.5f},
            .normal = glm::vec3{-1.0f, 0.0f, 0.0f},
            .uv = glm::vec2{0.0f, 0.0f}
        },

        Vertex{
            .position = glm::vec3{0.5f, 0.5f, 0.5f},
            .normal = glm::vec3{1.0f, 0.0f, 0.0f},
            .uv = glm::vec2{1.0f, 0.0f}
        },
        Vertex{
            .position = glm::vec3{0.5f, 0.5f, -0.5f},
            .normal = glm::vec3{1.0f, 0.0f, 0.0f},
            .uv = glm::vec2{1.0f, 1.0f}
        },
        Vertex{
            .position = glm::vec3{0.5f, -0.5f, -0.5f},
            .normal = glm::vec3{1.0f, 0.0f, 0.0f},
            .uv = glm::vec2{0.0f, 1.0f}
        },
        Vertex{
            .position = glm::vec3{0.5f, -0.5f, 0.5f},
            .normal = glm::vec3{1.0f, 0.0f, 0.0f},
            .uv = glm::vec2{0.0f, 0.0f}
        },

        Vertex{
            .position = glm::vec3{-0.5f, -0.5f, -0.5f},
            .normal = glm::vec3{0.0f, -1.0f, 0.0f},
            .uv = glm::vec2{0.0f, 1.0f}
        },
        Vertex{
            .position = glm::vec3{0.5f, -0.5f, -0.5f},
            .normal = glm::vec3{0.0f, -1.0f, 0.0f},
            .uv = glm::vec2{1.0f, 1.0f}
        },
        Vertex{
            .position = glm::vec3{0.5f, -0.5f, 0.5f},
            .normal = glm::vec3{0.0f, -1.0f, 0.0f},
            .uv = glm::vec2{1.0f, 0.0f}
        },
        Vertex{
            .position = glm::vec3{-0.5f, -0.5f, 0.5f},
            .normal = glm::vec3{0.0f, -1.0f, 0.0f},
            .uv = glm::vec2{0.0f, 0.0f}
        },

        Vertex{
            .position = glm::vec3{-0.5f, 0.5f, -0.5f},
            .normal = glm::vec3{0.0f, 1.0f, 0.0f},
            .uv = glm::vec2{0.0f, 1.0f}
        },
        Vertex{
            .position = glm::vec3{0.5f, 0.5f, -0.5f},
            .normal = glm::vec3{0.0f, 1.0f, 0.0f},
            .uv = glm::vec2{1.0f, 1.0f}
        },
        Vertex{
            .position = glm::vec3{0.5f, 0.5f, 0.5f},
            .normal = glm::vec3{0.0f, 1.0f, 0.0f},
            .uv = glm::vec2{1.0f, 0.0f}
        },
        Vertex{
            .position = glm::vec3{-0.5f, 0.5f, 0.5f},
            .normal = glm::vec3{0.0f, 1.0f, 0.0f},
            .uv = glm::vec2{0.0f, 0.0f}
        },
    };

    geo.indices = {
        2,  1,  0,  0,  3,  2,

        4,  5,  6,  6,  7,  4,

        8,  9,  10, 10, 11, 8,

        14, 13, 12, 12, 15, 14,

        16, 17, 18, 18, 19, 16,

        22, 21, 20, 20, 23, 22,
    };

    return geo;
}

Geometry createUnitSphereGeometry(uint32_t sectors, uint32_t stacks) {
    std::vector<Vertex> vertices;
    std::vector<Index> indices;

    const float sectorStep =
        2.0F * std::numbers::pi_v<float> / static_cast<float>(sectors);
    const float stackStep =
        std::numbers::pi_v<float> / static_cast<float>(stacks);

    // Generate vertices
    for (auto i : std::views::iota(0U, stacks + 1)) {
        const float stackAngle = (std::numbers::pi_v<float> / 2.0F) -
                                 (static_cast<float>(i) * stackStep);
        const float xy = std::cos(stackAngle);
        const float z = std::sin(stackAngle);

        for (auto j : std::views::iota(0U, sectors + 1)) {
            const float sectorAngle = static_cast<float>(j) * sectorStep;

            const float x = xy * std::cos(sectorAngle);
            const float y = xy * std::sin(sectorAngle);

            vertices.push_back(
                Vertex{
                    .position = glm::vec3{x, y, z},
                    .normal = glm::vec3{x, y, z},
                    .uv = glm::vec2{
                        static_cast<float>(j) / static_cast<float>(sectors),
                        static_cast<float>(i) / static_cast<float>(stacks)
                    }
                }
            );
        }
    }

    // Generate indices
    for (auto i : std::views::iota(0U, stacks)) {
        uint32_t k1 = i * (sectors + 1);
        uint32_t k2 = k1 + sectors + 1;

        for ([[maybe_unused]] auto j : std::views::iota(0U, sectors)) {
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }
            if (i != stacks - 1) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
            ++k1;
            ++k2;
        }
    }

    return {.vertices = std::move(vertices), .indices = std::move(indices)};
}
