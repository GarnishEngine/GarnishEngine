#include "modifiedECS.h"

#include <Rendering/OpenGL/shader_program.hpp>
#include <Physics/physics_system.hpp>

int main() {
    garnish::App app{};

    // auto i = app.get_controller().register_system<ImGuiSystem>(0);
    auto c = app.get_controller().register_system<CameraSystem>(0);

    app.get_controller().register_component<Camera>();
    app.get_controller().register_component<Renderable>();
    app.get_controller().register_component<Transform>();

    auto camera_entity =
        app.get_controller().create_entity_with_components(Camera());

    auto meshInstance =
        app.get_render_device()->setup_mesh("Models/viking_room.obj");
    auto tex =
        app.get_render_device()->load_texture("Textures/viking_room.png");

    app.get_controller().create_entity_with_components(Camera());

    constexpr float MODEL_ROT_X_DEGREES = -90.0F;
    constexpr float MODEL_ROT_Z_DEGREES = -135.0F;
    constexpr float MODEL_POS_X = 0.0F;
    constexpr float MODEL_POS_Y = -0.3F;
    constexpr float MODEL_POS_Z = 3.0F;
    const glm::quat qX = glm::angleAxis(glm::radians(MODEL_ROT_X_DEGREES), glm::vec3{1,0,0});
    const glm::quat qZ = glm::angleAxis(glm::radians(MODEL_ROT_Z_DEGREES), glm::vec3{0,0,1});
    auto vikingRoom = app.get_controller().create_entity_with_components(
        Renderable{.meshHandle = meshInstance, .texHandle = tex},
        Transform{ .position = {MODEL_POS_X, MODEL_POS_Y, MODEL_POS_Z}, .rotation = qX * qZ }
    );

    app.run();
}
