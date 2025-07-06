#include "pong.h"

#include <cstdint>

#include "shader_program.hpp"
#include "vulkan_renderer.hpp"

int main() {
    garnish::App app{
        {.backend = RenderingBackend::Vulkan,
         .width = 800,
         .height = 600,
         .targetFps = 144}
    };
    app.get_controller().register_component<Renderable>();

    uint32_t meshHandle =
        app.get_render_device()->setup_mesh("Models/viking_room.obj");

    uint32_t texHandle =
        app.get_render_device()->load_texture("Textures/viking_room.png");

    auto vikingRoom = app.get_controller().create_entity_with_components(
        Renderable{.meshHandle = meshHandle, .texHandle = texHandle}
    );

    // // TODO 3d mesh
    //  = app.get_controller().create_entity();

    // garnish::Mesh gMesh;
    // garnish::Texture texture;
    // texture.load_texture();

    app.run();
}
