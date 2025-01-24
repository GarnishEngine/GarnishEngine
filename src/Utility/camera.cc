#include "camera.hpp"

namespace garnish {
    glm::mat4 Camera::ViewMatrix() {
        return glm::lookAt(position, position + frontVector, upVector);
    }
}