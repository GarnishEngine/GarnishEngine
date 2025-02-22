#pragma once


#include "garnish_entity.hpp"
#include "garnish_window.hpp"
#include <glm/ext/matrix_clip_space.hpp>


#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"

#include "garnish_ecs.h"

#include <memory>
#include <chrono>

typedef std::chrono::high_resolution_clock hrclock;
typedef std::chrono::time_point<hrclock> tp;
typedef std::chrono::milliseconds ms;
typedef std::chrono::microseconds us;

using std::chrono::duration_cast;

namespace garnish {
    class app {
    public:
        int32_t WIDTH;
        int32_t HEIGHT;

        app(int32_t w, int32_t h);
        app();
        ~app() {}

        void run();

        bool shouldClose() { return garnishWindow.shouldClose; }
        virtual bool handle_poll_event();
        void handle_all_events();

        window garnishWindow;

        ECSManager ecsManager{ };
    };
}
