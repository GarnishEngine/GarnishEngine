#pragma once 

#include "SDL3/SDL_events.h"

namespace garnish {
    class Event {
        public:
        Event(SDL_Event &SDLEvent);
        Event();
        SDL_Event sdl_event;

        bool state;
    };
}