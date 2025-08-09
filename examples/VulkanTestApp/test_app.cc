#include "test_app.h"

#include <cstdint>
#include <ecs_controller.h>
#include <shared.hpp>
// Include via rendering namespace path rooted at src/Rendering
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

    auto camera_entity =
        app.get_controller().create_entity_with_components(Camera());

    uint32_t meshHandle =
        app.get_render_device()->setup_mesh("Models/viking_room.obj");

    uint32_t texHandle =
        app.get_render_device()->load_texture("Textures/viking_room.png");

    auto vikingRoom = app.get_controller().create_entity_with_components(
        Renderable{.meshHandle = meshHandle, .texHandle = texHandle}
    );

    app.run();
}
