#pragma once 

#include "garnish_event.hpp"

namespace garnish {
    class GarnishEntity {
        public:
          virtual ~GarnishEntity() = default;
          virtual void update(GarnishEvent &gEvent) = 0;
    };

}