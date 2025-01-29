#include "camera.hpp"


namespace garnish {
    glm::mat4 Camera::ViewMatrix() {
        return glm::lookAt(position, position + forward, up);
    }

    void Camera::update(event &gEvent) {
        // debug("cam update");
        // debug(position);
        
        switch (gEvent.sdl_event.type) {
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (gEvent.sdl_event.button.button == 1) {
                    mouseButtonHeld = true;
                }
                break;

            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (gEvent.sdl_event.button.button == 1) {
                    mouseButtonHeld = false;
                }
                break;

            case SDL_EVENT_MOUSE_MOTION:
                if (!mouseButtonHeld) {
                    break;
                }

                yaw += gEvent.sdl_event.motion.xrel * lookSensitivity;
                pitch -= gEvent.sdl_event.motion.yrel * lookSensitivity;

                if (pitch > 89.9f) {
                    pitch = 89.9f;
                }
                else if (pitch < -89.9f) {
                    pitch = -89.9f;
                }

                forward.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
                forward.y = sin(glm::radians(pitch));
                forward.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
                forward = glm::normalize(forward);

                right = glm::normalize(glm::cross(forward, up));

                break;
        }
    }
    void Camera::update() {
        if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_W]) {
            position += forward * movementSpeed;
        }
        if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_S]) {
            position -= forward * movementSpeed;
        }
        if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_D]) {
            position += right * (movementSpeed / 3);
        }
        if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_A]) {
            position -= right * (movementSpeed / 3);
        }
        if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_SPACE]) {
            position += up * (movementSpeed);
        }
        if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LSHIFT]) {
            position -= up * (movementSpeed);
        }
    }
    Camera::Camera(float movementSpeed, float lookSensitivity) 
        : movementSpeed(movementSpeed), lookSensitivity(lookSensitivity) {

        glm::vec3 position{0.0f, 0.0f, 5.0f};

        glm::vec3 up{0.0f, 1.0f, 0.0f};
        glm::vec3 forward{0.0f, 0.0f, -1.0f};
        glm::vec3 right{1.0f, 0.0f, 0.0f};
    }
}