#include "garnish_app.hpp"

namespace garnish {
    void GarnishApp::run() {
        SDL_Event event;

        while (!shouldClose()) {
            SDL_PollEvent(&event);
            if (event.window.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                carrotWindow.shouldClose = true;
            }
        }
    }
}
