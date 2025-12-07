#include "physics_system.hpp"
#include <chrono>
#include <string>
#include <iostream>
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

    auto collision_bodies = world.get_entities<Transform, RigidBody, SphereCollider>();

    for (Entity& a : collision_bodies) {
        for (Entity& b : collision_bodies) {
            if (a == b) continue;

            auto& tA = world.get_component<Transform>(a);
            auto& rbA = world.get_component<RigidBody>(a);
            auto& scA = world.get_component<SphereCollider>(a);

            auto& tB = world.get_component<Transform>(b);
            auto& rbB = world.get_component<RigidBody>(b);
            auto& scB = world.get_component<SphereCollider>(b);

            collide(tA, tB, rbA, rbB, scA, scB);
        }
    }
}

void PhysicsSystem::integrate(const float dt, RigidBody& rb, Transform& tf) {
    log_timed(std::to_string(tf.position.x) + " " + std::to_string(tf.position.y) + " " + std::to_string(tf.position.z) + " " + std::to_string(dt));
    tf.position += rb.velocity * dt + rb.acceleration * dt * dt * (1.0F / 2.0F);
    rb.velocity += rb.acceleration * dt;
    rb.velocity *= std::pow(rb.dampening, dt);
}

void PhysicsSystem::collide(Transform& tA, Transform& tB, RigidBody& rbA, RigidBody& rbB, SphereCollider& scA, SphereCollider& scB) {
    Collision col = findCollision(tA, tB, rbA, rbB, scA, scB);

    if (!col.hasCollision) return;

    float speedDotNormal = glm::dot(rbA.velocity - rbB.velocity, col.normal);

    // This is important for convergence a negitive impulse would drive the objects closer together
    if (speedDotNormal >= 0) return;

    float e = scA.restitutionCoefficient * scB.restitutionCoefficient;

    float j = -(1.0f + e) * speedDotNormal / (rbA.inv_mass + rbB.inv_mass);

    glm::vec3 impulse = j * col.normal;

    rbA.velocity += impulse * rbA.inv_mass;
    rbB.velocity -= impulse * rbB.inv_mass;
}

Collision PhysicsSystem::findCollision(Transform& tA, Transform& tB, RigidBody& rbA, RigidBody& rbB, SphereCollider& scA, SphereCollider& scB) {
    Collision col{ };

    float distance = glm::distance(tA.position, tB.position);

    if (distance > scA.radius + scB.radius) {
        // No collision
        col.hasCollision = false;

        return col;
    }

    // Collision happend
    col.hasCollision = true;

    glm::vec3 AtoB = tB.position - tA.position;
    AtoB = glm::normalize(AtoB);
    col.a = tA.position + AtoB * scA.radius;

    glm::vec3 BtoA = tA.position - tB.position;
    BtoA = glm::normalize(BtoA);
    col.b = tB.position + BtoA * scB.radius;

    col.normal = glm::normalize(col.b - col.a);
    col.depth = glm::length(col.b - col.a);

    return col;
}
}  // namespace garnish