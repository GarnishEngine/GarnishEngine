#include "camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace garnish {
    glm::mat4 Camera::ViewMatrix() {
        return glm::lookAt(position, position + forward, up);
    }

    void Camera::update(GarnishEvent &gEvent) {
        // debug("cam update");
        // debug(position);
        switch (gEvent.event.type) {
            case SDL_EVENT_KEY_DOWN:
                if (gEvent.event.key.scancode == SDL_SCANCODE_W) {
                    position += forward * speed;
                }
                if (gEvent.event.key.scancode == SDL_SCANCODE_S) {
                    position -= forward * speed;
                }
                if (gEvent.event.key.scancode == SDL_SCANCODE_D) {
                    position += right * speed;
                }
                if (gEvent.event.key.scancode == SDL_SCANCODE_A) {
                    position -= right * speed;
                }
                break;
              default:
                break;
        }
    }
    Camera::Camera(float s) {
        glm::vec3 position{0.0f, 0.0f, 5.0f};

        glm::vec3 up{0.0f, 1.0f, 0.0f};
        glm::vec3 forward{0.0f, 0.0f, -1.0f};
        glm::vec3 right{1.0f, 0.0f, 0.0f};

        speed = s;
    }
}