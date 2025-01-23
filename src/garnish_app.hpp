#pragma once

#include "carrot_window.hpp"
#include "sage_pipeline.hpp"

namespace garnish {
    class GarnishApp {
      public:
        static constexpr int WIDTH = 800;
        static constexpr int HEIGHT = 600;

        void run();
        bool shouldClose() { return carrotWindow.shouldClose; }
      private:
        CarrotWindow carrotWindow{WIDTH, HEIGHT, "Hello"};
        SagePipeline sagePipeline{"shaders/shad.vert", "shaders/shad.frag"};
    };
    
}