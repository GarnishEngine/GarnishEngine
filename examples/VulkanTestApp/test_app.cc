#include "test_app.h"

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

    uint32_t meshHandle = dynamic_cast<garnish::VulkanRenderDevice*>(
                              app.get_render_device().get()
    )
                              ->setup_mesh("Models/viking_room.obj");

    uint32_t texHandle = dynamic_cast<garnish::VulkanRenderDevice*>(
                             app.get_render_device().get()
    )
                             ->create_texture_image("Textures/viking_room.png");

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
