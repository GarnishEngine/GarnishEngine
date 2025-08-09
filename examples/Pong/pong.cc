#include "pong.h"

#include <cstdint>
#include <shared.hpp>
#include <VulkanBackend/vulkan_renderer.hpp>

int main() {
    garnish::App app{
        {.backend = RenderingBackend::Vulkan,
         .width = garnish::App::DEFAULT_WIDTH,
         .height = garnish::App::DEFAULT_HEIGHT,
         .targetFps = garnish::App::DEFAULT_TARGET_FPS}
    };
    app.get_controller().register_component<Renderable>();

    uint32_t meshHandle =
        app.get_render_device()->setup_mesh("Models/viking_room.obj");

    uint32_t texHandle =
        app.get_render_device()->load_texture("Textures/viking_room.png");

    auto vikingRoom = app.get_controller().create_entity_with_components(
        Renderable{.meshHandle = meshHandle, .texHandle = texHandle}
    );

    app.run();
}
