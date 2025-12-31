#pragma once
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace garnish {
struct Renderable {
    uint32_t meshHandle;
    uint32_t texHandle;
};
struct Transform {
    glm::vec3 position{0.0F};
    glm::quat rotation{1.0F, 0.0F, 0.0F, 0.0F};
};

struct Material {
    glm::vec3 ambient{0.1F, 0.1F, 0.1F};
    glm::vec3 diffuse{1.0F, 1.0F, 1.0F};
    glm::vec3 specular{0.5F, 0.5F, 0.5F};
    float shininess{32.0F};
};

struct PointLight {
    glm::vec3 position{0.0F, 5.0F, 5.0F};
    glm::vec3 color{1.0F, 1.0F, 1.0F};
    float intensity{1.0F};
};
}  // namespace garnish