#pragma once 


#include "garnish_event.hpp"
#include "SDL3/SDL_events.h"
#include <SDL3/SDL_scancode.h>

namespace garnish {
    class GarnishEntity {
        public:
          virtual ~GarnishEntity() = default;
          virtual void update(GarnishEvent &gEvent) = 0;
    };

}