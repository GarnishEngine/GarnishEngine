#pragma once

#include <algorithm>
#include <garnish_app.hpp>
#include <ecs_controller.h>
#include <system.h>
#include <limits>
#include <Utility/camera.hpp>
#include <shared.hpp>
#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_mouse.h"

namespace {
constexpr int32_t FRAME_RATE = 90;
float yaw = 0.0f;
float pitch = 0.0f;
float x = 0.0f;
float y = 0.0f;
}

class CameraSystem : public garnish::System {
    public:
     void update(garnish::ECSController& world) override {
        auto cam_ent = world.get_entities<garnish::Camera>()[0];  // TODO this is really janky, need to do
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


    }
};
