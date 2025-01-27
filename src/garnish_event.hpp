#pragma once 

#include "SDL3/SDL_events.h"

namespace garnish {
    class GarnishEvent {
        public:
        GarnishEvent(SDL_Event &SDLEvent);
        GarnishEvent();
        SDL_Event event;

        bool state;


    };
}