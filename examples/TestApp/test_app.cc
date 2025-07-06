#include "test_app.h"

#include "ogl_renderer.hpp"
#include "shader_program.hpp"

int main() {
    garnish::App app{};

    // auto i = app.get_controller().register_system<ImGuiSystem>(0);
    auto c = app.get_controller().register_system<CameraSystem>(0);

    app.get_controller().register_component<Camera>();
    app.get_controller().register_component<Renderable>();

    auto camera_entity =
        app.get_controller().create_entity_with_components(Camera());

    auto meshInstance =
        app.get_render_device()->setup_mesh("Models/viking_room.obj");
    auto tex =
        app.get_render_device()->load_texture("Textures/viking_room.png");

    app.get_controller().create_entity_with_components(Camera());

    auto vikingRoom = app.get_controller().create_entity_with_components(
        Renderable{.meshHandle = meshInstance, .texHandle = tex}
    );

    app.run();
}
