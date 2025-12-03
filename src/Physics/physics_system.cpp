#include "physics_system.hpp"
#include <chrono>
#include <string>
#include "Utility/log.hpp"
#include "ecs_common.h"

namespace garnish {


void PhysicsSystem::update(ECSController& world) {
    auto rigid_bodies = world.get_entities<Transform, RigidBody>();
    using namespace std::chrono;

    auto now = clock::now();
    duration<float> chrono_dt = now-time;
    float dt = chrono_dt.count();

    time = now;
    if (dt > 1.0F) return;
    
    for (Entity &entity : rigid_bodies) {
        auto& rb = world.get_component<RigidBody>(entity);
        auto& tf = world.get_component<Transform>(entity);

        integrate(dt, rb, tf);
    }
}
void PhysicsSystem::integrate(const float dt, RigidBody& rb, Transform& tf) {
    log_timed(std::to_string(tf.position.x) + " " + std::to_string(tf.position.y) + " " + std::to_string(tf.position.z) + " " + std::to_string(dt));
    tf.position +=  rb.velocity * dt +
                rb.acceleration * dt * dt * (1.0F / 2.0F);
    rb.velocity += rb.acceleration * dt;
    rb.velocity *= std::pow(rb.dampening, dt);
}
}  // namespace garnish