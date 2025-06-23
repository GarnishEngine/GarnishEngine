#pragma once

#include <SDL3/SDL_keyboard.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <limits>

namespace garnish {
struct Camera {
    Camera(float movementSpeed = 0.02f, float lookSensitivity = 0.5f);

    glm::vec3 position{0.0f, 0.0f, 5.0f};

    glm::vec3 up{0.0f, 1.0f, 0.0f};
    glm::vec3 forward{0.0f, 0.0f, -1.0f};
    glm::vec3 right{1.0f, 0.0f, 0.0f};

    float yaw{-90.0f};
    float pitch{0.0f};

    float movementSpeed;
    float lookSensitivity;
    bool held = false;

    glm::vec2 lastMousePos{std::numeric_limits<float>::max()};

    glm::mat4 view_matrix();
};
}  // namespace garnish
