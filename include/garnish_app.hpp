#pragma once
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include "garnish_window.hpp"
#include <glm/ext/matrix_clip_space.hpp>
#include "shader_program.hpp"
#include "camera.hpp"
#include "garnish_mesh.hpp"
#include "garnish_sprite.hpp"
#include "ecs_controller.h"
#include <memory>
#include <chrono>
#include <stdexcept>

namespace garnish {
    using hrclock = std::chrono::high_resolution_clock;
    using tp = std::chrono::time_point<hrclock>;
    using ms = std::chrono::milliseconds;
    using us = std::chrono::microseconds;

    using std::chrono::duration_cast;
    class App {
    public:
        App(int32_t w, int32_t h);
        App();
        virtual ~App();
        App(const App&) = delete;
        App &operator=(const App&) = delete;
        App(App&&) = delete;
        App &operator=(App&&) = delete;
        ECSController ecsController;

        void run();
        virtual bool handle_poll_event();
        void handle_all_events();
    protected:
        virtual void init();
        void init_imgui();
        void terminate_imgui();
        bool shouldClose = false;
    private:
        int32_t WIDTH;
        int32_t HEIGHT;

        Window garnishWindow;
    };
}
