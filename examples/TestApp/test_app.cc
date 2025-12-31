#include "test_app.h"

#include <imgui.h>

#include <Physics/physics_system.hpp>
#include <Rendering/OpenGL/shader_program.hpp>

void draw_debug_panel(garnish::ECSController& world) {
    ImGui::Begin("Debug Panel");

    ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("FPS: %.1f", io.Framerate);
    ImGui::Text("Frame Time: %.3f ms", 1000.0f / io.Framerate);

    auto entities = world.get_entities<Renderable>();
    ImGui::Text("Renderable Entities: %zu", entities.size());

    auto cameras = world.get_entities<garnish::Camera>();
    if (!cameras.empty()) {
        auto& cam = world.get_component<garnish::Camera>(cameras[0]);
        ImGui::Separator();
        ImGui::Text("Camera Position:");
        ImGui::Text("  X: %.2f", cam.position.x);
        ImGui::Text("  Y: %.2f", cam.position.y);
        ImGui::Text("  Z: %.2f", cam.position.z);
        ImGui::Text("Yaw: %.1f, Pitch: %.1f", cam.yaw, cam.pitch);
    }

    ImGui::End();
}

int main() {
    garnish::App app{};

    app.enable_imgui(true);

    app.register_imgui_callback([](garnish::ECSController& world) {
        ImGui::ShowDemoWindow();
        draw_debug_panel(world);
    });

    app.register_update_function([](garnish::ECSController& world) {
        CameraSystem().update(world);
    });

    auto camera_entity =
        app.get_controller().create_entity_with_components(Camera());

    auto meshInstance =
        app.get_render_device()->setup_mesh("Models/viking_room.obj");
    auto tex =
        app.get_render_device()->load_texture("Textures/viking_room.png");

    constexpr float MODEL_ROT_X_DEGREES = -90.0F;
    constexpr float MODEL_ROT_Z_DEGREES = -135.0F;
    constexpr float MODEL_POS_X = 0.0F;
    constexpr float MODEL_POS_Y = -0.3F;
    constexpr float MODEL_POS_Z = 3.0F;
    const glm::quat qX =
        glm::angleAxis(glm::radians(MODEL_ROT_X_DEGREES), glm::vec3{1, 0, 0});
    const glm::quat qZ =
        glm::angleAxis(glm::radians(MODEL_ROT_Z_DEGREES), glm::vec3{0, 0, 1});
    auto vikingRoom = app.get_controller().create_entity_with_components(
        Renderable{.meshHandle = meshInstance, .texHandle = tex},
        Transform{
            .position = {MODEL_POS_X, MODEL_POS_Y, MODEL_POS_Z},
            .rotation = qX * qZ
        }
    );

    app.run();
}
