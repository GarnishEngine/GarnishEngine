#include "camera.hpp"

namespace garnish {
Camera::Camera(float movementSpeed, float lookSensitivity)
    : movementSpeed(movementSpeed),
      lookSensitivity(lookSensitivity) {
}

glm::mat4 Camera::view_matrix() {
    return glm::lookAt(position, position + forward, up);
}
}  // namespace garnish
