#include "pong.h"


int main() {
    garnish::App app{ };

    auto i = app.ecsController.register_system<ImGuiSystem>(0);
    auto c = app.ecsController.register_system<CameraSystem>(0);
    auto m = app.ecsController.register_system<MeshSystem>(0);

    app.ecsController.register_component<Camera>();
    app.ecsController.register_component<Mesh>();
    app.ecsController.register_component<Sprite>();

    auto camera_entity = app.ecsController.create_entity_with_components(Camera());


    auto vikingRoom = app.ecsController.create_entity();

    garnish::Mesh gMesh;
    garnish::Texture texture;
    texture.load_texture("Textures/viking_room.png");

    gMesh.load_mesh("Models/Untitled.obj");
    gMesh.setup_mesh();
    gMesh.load_texture(texture);

    app.ecsController.add_component(vikingRoom, gMesh);



    app.run();
}
