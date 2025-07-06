#include <algorithm>
#include <garnish_app.hpp>
#include <limits>

#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_mouse.h"
#include "camera.hpp"

const int32_t FRAME_RATE = 90;

using namespace garnish;
static float yaw = 0.0f;
static float pitch = 0;
static float x = 0;
static float y = 0;
class CameraSystem : public System {
   public:
    int32_t WIDTH = 600;
    int32_t HEIGHT = 800;

    void update(ECSController& world) override {
        auto cam_ent = world.get_entities<
            garnish::Camera>()[0];  // TODO this is really janky, need to do
                                    // something about the camera
        auto& cam = world.get_component<garnish::Camera>(cam_ent);
        if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_W]) {
            cam.position += cam.forward * cam.movementSpeed;
        }
        if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_S]) {
            cam.position -= cam.forward * cam.movementSpeed;
        }
        if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_D]) {
            cam.position += cam.right * cam.movementSpeed;
        }
        if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_A]) {
            cam.position -= cam.right * cam.movementSpeed;
        }
        if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_SPACE]) {
            cam.position += cam.up * cam.movementSpeed;
        }
        if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LSHIFT]) {
            cam.position -= cam.up * cam.movementSpeed;
        }
        if (SDL_GetMouseState(nullptr, nullptr) == SDL_BUTTON_LEFT) {
            if (cam.lastMousePos ==
                glm::vec2(std::numeric_limits<float>::max())) {
                glm::vec2 mousePos{};
                SDL_GetMouseState(&mousePos.x, &mousePos.y);

                cam.lastMousePos = mousePos;
            }
            if (!cam.held) {
                glm::vec2 mousePos{};
                SDL_GetMouseState(&mousePos.x, &mousePos.y);
                cam.lastMousePos = mousePos;
            }
            cam.held = true;

            glm::vec2 mousePos{};
            SDL_GetMouseState(&mousePos.x, &mousePos.y);

            glm::vec2 deltaMousePos = cam.lastMousePos - mousePos;
            deltaMousePos /= 2;

            cam.yaw -= deltaMousePos.x * cam.lookSensitivity;
            cam.pitch += deltaMousePos.y * cam.lookSensitivity;

            cam.pitch = std::clamp(cam.pitch, -89.9f, 89.9f);

            cam.forward.x =
                cos(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch));
            cam.forward.y = sin(glm::radians(cam.pitch));
            cam.forward.z =
                sin(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch));
            cam.forward = glm::normalize(cam.forward);

            cam.right = glm::normalize(glm::cross(cam.forward, cam.up));

            x = cam.position.x;
            y = cam.position.x;
            pitch = cam.pitch;
            yaw = cam.yaw;

            cam.lastMousePos = mousePos;
        } else {
            cam.held = false;
        }

        glm::mat4 model{1.0f};
        model = glm::translate(model, glm::vec3{0.0f, -0.3f, 2.0f});

        model = glm::rotate(
            model,
            glm::radians(-90.0f),
            glm::vec3{1.0f, 0.0f, 0.0f}
        );
        model = glm::rotate(
            model,
            glm::radians(-135.0f),
            glm::vec3{0.0f, 0.0f, 1.0f}
        );

        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),
            (float)WIDTH / (float)HEIGHT,
            0.1f,
            10.0f
        );
        projection[1][1] *= -1;

        glm::mat4 mvp = projection * cam.view_matrix() * model;
        world.get<RenderDevice*>()->set_uniform(mvp);
    }
};
