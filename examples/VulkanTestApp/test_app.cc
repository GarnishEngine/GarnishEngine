#include "test_app.h"

#include <ecs_controller.h>
#include <imgui.h>

#include <Physics/physics_system.hpp>
#include <VulkanBackend/vulkan_renderer.hpp>
#include <cstdint>
#include <shared.hpp>

using namespace garnish;

namespace {
void draw_debug_panel(garnish::ECSController& world) {
    ImGui::Begin("Debug Panel");

    ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("FPS: %.1f", io.Framerate);
    ImGui::Text("Frame Time: %.3f ms", 1000.0F / io.Framerate);

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
}  // namespace

int main() {
    garnish::App app{
        {.backend = RenderingBackend::Vulkan,
         .width = garnish::App::DEFAULT_WIDTH,
         .height = garnish::App::DEFAULT_HEIGHT,
         .targetFps = garnish::App::DEFAULT_TARGET_FPS}
    };

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

    uint32_t meshHandle =
        app.get_render_device()->setup_mesh("Models/viking_room.obj");

    uint32_t texHandle =
        app.get_render_device()->load_texture("Textures/viking_room.png");

    constexpr float MODEL_ROT_X_DEGREES = -90.0F;
    constexpr float MODEL_ROT_Z_DEGREES = -135.0F;
    constexpr float MODEL_POS_X = 0.0F;
    constexpr float MODEL_POS_Y = -0.3F;
    constexpr float MODEL_POS_Z = 2.0F;

    const glm::quat qX =
        glm::angleAxis(glm::radians(MODEL_ROT_X_DEGREES), glm::vec3{1, 0, 0});
    const glm::quat qZ =
        glm::angleAxis(glm::radians(MODEL_ROT_Z_DEGREES), glm::vec3{0, 0, 1});
    auto vikingRoom = app.get_controller().create_entity_with_components(
        Renderable{.meshHandle = meshHandle, .texHandle = texHandle},
        Transform{
            .position = {MODEL_POS_X, MODEL_POS_Y, MODEL_POS_Z},
            .rotation = qX * qZ
        },
        RigidBody{
            .velocity = glm::vec3(0.1F),
            .acceleration = glm::vec3(0.0F),
            .inv_mass = 1.0F,
            .dampening = 1.0F
        }
    );

    app.run();
}
