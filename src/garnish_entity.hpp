#pragma once 

#include "garnish_event.hpp"

namespace garnish {
    class entity {
        public:
          virtual ~entity() = default;
          virtual void update(event &gEvent) = 0;
          virtual void update() = 0;
    };

}