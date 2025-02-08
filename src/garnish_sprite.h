#pragma once




#include "Rendering/garnish_texture.hpp"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>

namespace garnish {
    class sprite {
        public:
        sprite();

        void drawSprite();
        private:
        glm::vec2 position;
        glm::vec2 size;
        glm::vec3 color;
        float rotate;

        garnish_texture texture;
        unsigned int quadVAO;
    };
}
