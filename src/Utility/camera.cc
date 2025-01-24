#include "camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace garnish {
    glm::mat4 Camera::ViewMatrix() {
        return glm::lookAt(position, position + forward, up);
    }
}