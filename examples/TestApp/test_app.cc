#include "test_app.h"
#include "ogl_renderer.hpp"


int main() {
    garnish::App app{ };

    auto i = app.get_controller().register_system<ImGuiSystem>(0);
    auto c = app.get_controller().register_system<CameraSystem>(0);
    // auto m = app.get_controller().register_system<MeshSystem>(0);
    // auto s = app.get_controller().register_system<SpriteSystem>(0);

    app.get_controller().register_component<Camera>();
    app.get_controller().register_component<drawable>();
    app.get_controller().register_component<texture>();

    auto camera_entity = app.get_controller().create_entity_with_components(Camera());

    auto mesH =
        dynamic_cast<garnish::OpenGLRenderDevice*>(app.getRenderDevice().get())
            ->setup_mesh("Models/viking_room.obj");
    auto tex =
        dynamic_cast<garnish::OpenGLRenderDevice*>(app.getRenderDevice().get())
            ->load_texture("Textures/viking_room.png");

    app.get_controller().create_entity_with_components(Camera());

    auto vikingRoom = app.get_controller().create_entity_with_components(
        drawable{
            .VAO = static_cast<int>(mesH.VAO),
            .size = static_cast<int>(mesH.numIndicies)
        },
        tex
    );

    // // TODO 3d mesh
    //  = app.get_controller().create_entity();


    // garnish::Mesh gMesh;
    // garnish::Texture texture;
    // texture.load_texture();



    app.run();
}
