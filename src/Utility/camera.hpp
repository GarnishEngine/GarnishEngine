#pragma once

#include <SDL3/SDL_keyboard.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <limits>

namespace garnish {
struct Camera {
    Camera(float movementSpeed = 0.02F, float lookSensitivity = 0.5F);

    glm::vec3 position{0.0F, 0.0F, 5.0F};

    glm::vec3 up{0.0F, 1.0F, 0.0F};
    glm::vec3 forward{0.0F, 0.0F, -1.0F};
    glm::vec3 right{1.0F, 0.0F, 0.0F};

    float yaw{-90.0F};
    float pitch{0.0F};

    float movementSpeed;
    float lookSensitivity;
    bool held = false;

    glm::vec2 lastMousePos{std::numeric_limits<float>::max()};

    glm::mat4 view_matrix();
};
}  // namespace garnish
