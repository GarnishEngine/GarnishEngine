#include "garnish_event.hpp"
#include "SDL3/SDL_events.h"

#include <iostream>
namespace garnish {
    GarnishEvent::GarnishEvent(SDL_Event &SDLEvent) : event(SDLEvent) {}
    GarnishEvent::GarnishEvent() {
        state = SDL_PollEvent(&event);   
    }
}