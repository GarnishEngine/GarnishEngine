#pragma once

#include "Rendering/OpenGL/gl_buffer.hpp"
#include "Rendering/OpenGL/shader_program.hpp"
#include "Rendering/garnish_mesh.hpp"
#include "Rendering/graphics_pipeline.hpp"
#include "SDL3/SDL_events.h"
#include "Utility/camera.hpp"
#include "garnish_entity.hpp"
#include "garnish_event.hpp"
#include "garnish_window.hpp"
#include <glm/ext/matrix_clip_space.hpp>

#include <memory>
#include <chrono>

typedef std::chrono::high_resolution_clock hrclock;
typedef std::chrono::time_point<hrclock> tp;
typedef std::chrono::milliseconds ms;
using std::chrono::duration_cast;

namespace garnish {
    class GarnishApp {
    public:
        int32_t WIDTH;
        int32_t HEIGHT;

        GarnishApp(int32_t w, int32_t h);
        GarnishApp();
        ~GarnishApp() {}

        virtual void run() {}
        bool shouldClose() { return garnishWindow.shouldClose; }
        bool handle_poll_event();
        void handle_all_events();

        GarnishWindow garnishWindow;
        std::vector<std::shared_ptr<GarnishEntity>> entities;
    };
}
