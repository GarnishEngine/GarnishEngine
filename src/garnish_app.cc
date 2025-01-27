#include "garnish_app.hpp"

typedef std::chrono::duration<float> fsec;
namespace garnish {
    GarnishApp::GarnishApp(int32_t w, int32_t h) : WIDTH(w), HEIGHT(h), garnishWindow(w, h, "hello window") {}
    GarnishApp::GarnishApp() : WIDTH(800), HEIGHT(600), garnishWindow(800, 600, "hello window") {}

    bool GarnishApp::handle_poll_event() {
        GarnishEvent event{};
        if (!event.state)
            return false;
        if (event.event.window.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            garnishWindow.shouldClose = true;
        }
        if (event.event.window.type == SDL_EVENT_WINDOW_RESIZED) {
            garnishWindow.pairWindowSize(&WIDTH, &HEIGHT);
            glViewport(0, 0, WIDTH, HEIGHT);
        }  

        for (const auto &entity : entities) {
            entity->update(event);
        }

        return true;
    }
    
    void GarnishApp::handle_all_events() {
        while (handle_poll_event()) {}
    }

}
