#pragma once

#include <system.h>
#include <ecs_controller.h>
#include "glm/ext/vector_float3.hpp"
#include <chrono>
#include <shared.hpp>

namespace garnish {
struct RigidBody {
    glm::vec3 velocity;
    glm::vec3 acceleration;
    float inv_mass;
    float dampening;
};

class PhysicsSystem : public System {
   public:
    PhysicsSystem() = default;
    void update(ECSController& world) override;
   private:
    static void integrate(float dt, RigidBody& rb, Transform& tf);
    using clock = std::chrono::steady_clock;

    clock::time_point time;

};
}  // namespace garnish