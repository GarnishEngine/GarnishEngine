#pragma once

#include "garnish_window.hpp"
#include "graphics_pipeline.hpp"

namespace garnish {
    class GarnishApp {
        public:
            static constexpr int WIDTH = 800;
            static constexpr int HEIGHT = 600;

            void run();
            bool shouldClose() { return garnishWindow.shouldClose; }
        private:
            GarnishWindow garnishWindow{WIDTH, HEIGHT, "Hello"};
    };
}
