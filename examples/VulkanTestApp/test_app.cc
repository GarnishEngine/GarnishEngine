#include "test_app.h"

#include <cstdint>
#include <ecs_controller.h>
#include <shared.hpp>
#include <Physics/physics_system.hpp> 
#include <VulkanBackend/vulkan_renderer.hpp>

using namespace garnish; 

int main() {
    garnish::App app{
        {.backend = RenderingBackend::Vulkan,
         .width = garnish::App::DEFAULT_WIDTH,
         .height = garnish::App::DEFAULT_HEIGHT,
         .targetFps = garnish::App::DEFAULT_TARGET_FPS}
    };
    auto c = app.get_controller().register_system<CameraSystem>(0);

    app.get_controller().register_component<Renderable>();
    app.get_controller().register_component<Camera>();
    app.get_controller().register_component<Transform>();
    app.get_controller().register_component<RigidBody>();

    auto camera_entity =
        app.get_controller().create_entity_with_components(Camera());

    uint32_t meshHandle =
        app.get_render_device()->setup_mesh("Models/viking_room.obj");

    uint32_t texHandle =
        app.get_render_device()->load_texture("Textures/viking_room.png");

    constexpr float MODEL_ROT_X_DEGREES = -90.0F;
    constexpr float MODEL_ROT_Z_DEGREES = -135.0F; 
    constexpr float MODEL_POS_X = 0.0F;
    constexpr float MODEL_POS_Y = -0.3F;
    constexpr float MODEL_POS_Z = 2.0F;
    const glm::quat qX = glm::angleAxis(glm::radians(MODEL_ROT_X_DEGREES), glm::vec3{1,0,0});
    const glm::quat qZ = glm::angleAxis(glm::radians(MODEL_ROT_Z_DEGREES), glm::vec3{0,0,1});
    auto vikingRoom = app.get_controller().create_entity_with_components(
        Renderable{.meshHandle = meshHandle, .texHandle = texHandle},
        Transform{ .position = {MODEL_POS_X, MODEL_POS_Y, MODEL_POS_Z}, .rotation = qX * qZ },
        RigidBody{  
            .velocity = glm::vec3(0.1F), 
            .acceleration = glm::vec3(0.0F), 
            .inv_mass = 1.0F, 
            .dampening = 1.0F
        }
    );

    app.run();
}
