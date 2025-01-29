#pragma once

#include "../garnish_entity.hpp"
#include "../garnish_event.hpp"
#include <SDL3/SDL_keyboard.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>


namespace garnish {
    struct Camera : public entity {
        Camera(float movementSpeed = 0.05f, float lookSensitivity = 0.5f);
        
        glm::vec3 position{ 0.0f, 0.0f, 5.0f };

        glm::vec3 up{ 0.0f, 1.0f, 0.0f };
        glm::vec3 forward{ 0.0f, 0.0f, -1.0f };
        glm::vec3 right{ 1.0f, 0.0f, 0.0f };

        float yaw{ -90.0f };
        float pitch{ 0.0f };

        bool mouseButtonHeld{ false };

        glm::mat4 ViewMatrix();

        float movementSpeed;
        float lookSensitivity;

        void update(event &gEvent) override;
        void update() override;
    };
}