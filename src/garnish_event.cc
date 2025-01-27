#include "garnish_event.hpp"

namespace garnish {
    GarnishEvent::GarnishEvent(SDL_Event &SDLEvent) : event(SDLEvent) {}
    GarnishEvent::GarnishEvent() {
        state = SDL_PollEvent(&event);   
    }
}