#include "garnish_event.hpp"

namespace garnish {
    Event::Event(SDL_Event &SDLEvent) : sdl_event(SDLEvent) {}
    Event::Event() {
        state = SDL_PollEvent(&sdl_event);   
    }
}