#include "camera.hpp"

namespace garnish {
    Camera::Camera(float movementSpeed, float lookSensitivity) 
        : movementSpeed(movementSpeed), lookSensitivity(lookSensitivity) {

        glm::vec3 position{0.0f, 0.0f, 5.0f};

        glm::vec3 up{0.0f, 1.0f, 0.0f};
        glm::vec3 forward{0.0f, 0.0f, -1.0f};
        glm::vec3 right{1.0f, 0.0f, 0.0f};
    }

    glm::mat4 Camera::ViewMatrix() {
        return glm::lookAt(position, position + forward, up);
    }
}
