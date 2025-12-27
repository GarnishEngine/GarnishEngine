#pragma once

#include <ecs_controller.h>

#include <chrono>
#include <shared.hpp>

#include "glm/ext/vector_float3.hpp"

namespace garnish {
struct RigidBody {
    glm::vec3 velocity;
    glm::vec3 acceleration;
    float inv_mass;
    float dampening;
};

// Collision logic is based on: https://winter.dev/articles/physics-engine
struct Collision {
    glm::vec3 a;       // Furthest point of A into B
    glm::vec3 b;       // Furthest point of B into A
    glm::vec3 normal;  // B – A normalized
    float depth;       // Length of B – A
    bool hasCollision;
};

struct SphereCollider {
    float radius;
    float restitutionCoefficient;
};

class PhysicsSystem {
   public:
    PhysicsSystem() = default;
    void update(ECSController& world);

   private:
    static void integrate(float dt, RigidBody& rb, Transform& tf);
    using clock = std::chrono::steady_clock;

    clock::time_point time;

    static void collide(
        Transform& tA,
        Transform& tB,
        RigidBody& rbA,
        RigidBody& rbB,
        SphereCollider& scA,
        SphereCollider& scB
    );
    static Collision findCollision(
        Transform& tA,
        Transform& tB,
        RigidBody& rbA,
        RigidBody& rbB,
        SphereCollider& scA,
        SphereCollider& scB
    );
};
}  // namespace garnish