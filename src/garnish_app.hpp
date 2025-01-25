#pragma once

#include "SDL3/SDL_events.h"
#include "Utility/OpenGL/gl_buffer.hpp"
#include "Utility/OpenGL/shader_program.hpp"
#include "Utility/camera.hpp"
#include "garnish_entity.hpp"
#include "garnish_event.hpp"
#include "garnish_window.hpp"
#include "graphics_pipeline.hpp"
#include <glm/ext/matrix_clip_space.hpp>
#include <memory>

namespace garnish {
    class GarnishApp {
        public:
            static constexpr int WIDTH = 800;
            static constexpr int HEIGHT = 600;

            void run();
            bool shouldClose() { return garnishWindow.shouldClose; }
            void handle_poll_event();
        private:
            GarnishWindow garnishWindow{WIDTH, HEIGHT, "Hello"};
            std::vector<std::shared_ptr<GarnishEntity>> entities;
    };
}
