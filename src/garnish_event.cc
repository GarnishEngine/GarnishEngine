#include "garnish_event.hpp"

namespace garnish {
    event::event(SDL_Event &SDLEvent) : sdl_event(SDLEvent) {}
    event::event() {
        state = SDL_PollEvent(&sdl_event);   
    }
}