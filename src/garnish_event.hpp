#pragma once 

#include "SDL3/SDL_events.h"

namespace garnish {
    class event {
        public:
        event(SDL_Event &SDLEvent);
        event();
        SDL_Event sdl_event;

        bool state;


    };
}