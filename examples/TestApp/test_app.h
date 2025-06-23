#include "garnish.hpp"
#include <limits>
#include <memory>

const int32_t FRAME_RATE = 90;

typedef std::chrono::high_resolution_clock hrclock;
typedef std::chrono::time_point<hrclock> tp;
typedef std::chrono::milliseconds ms;
using std::chrono::duration_cast;
using namespace garnish;
static float yaw = 0.0f;
static float pitch = 0;
static float x = 0;
static float y = 0;



class ImGuiSystem : public System {
    public:
    void update(ECSController& world) override {
        ImGui::ShowDemoWindow();

        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!"); // Create a window called "Hello,
                                        // world!" and append into it.

        ImGui::Text(
            "This is some useful text."); // Display some text (you can use a
                                            // format strings too)

        if (ImGui::Button(
                "Button")) // Buttons return true when clicked (most widgets
                            // return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);
        ImGui::Text("pitch = %f", pitch);
        ImGui::Text("yaw = %f", yaw);
        ImGui::Text("x = %f", x);
        ImGui::Text("y = %f", y);

        ImGuiIO &io = ImGui::GetIO();
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                    1000.0f / io.Framerate, io.Framerate);
        ImGui::End();
    }
};
class CameraSystem : public System {
public:
    std::unique_ptr<ShaderProgram> shaderProgram;
    int32_t WIDTH = 600;
    int32_t HEIGHT = 800;

    CameraSystem() {
        shaderProgram = std::make_unique<ShaderProgram>("shaders/shader.vert", "shaders/shader.frag");
    }
    
    void update(ECSController& world) override {
        auto cam_ent = world.get_entities<garnish::Camera>()[0]; // TODO this is really janky, need to do something about the camera
        auto& cam = world.get_component<garnish::Camera>(cam_ent);
        if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_W]) {
            cam.position += cam.forward * cam.movementSpeed;
        }
        if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_S]) {
            cam.position -= cam.forward * cam.movementSpeed;
        }
        if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_D]) {
            cam.position += cam.right * cam.movementSpeed;
        }
        if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_A]) {
            cam.position -= cam.right * cam.movementSpeed;
        }
        if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_SPACE]) {
            cam.position += cam.up * cam.movementSpeed;
        }
        if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LSHIFT]) {
            cam.position -= cam.up * cam.movementSpeed;
        }
        if (SDL_GetMouseState(nullptr, nullptr) == SDL_BUTTON_LEFT) {
            if (cam.lastMousePos == glm::vec2(std::numeric_limits<float>::max())) {
                glm::vec2 mousePos{ };
                SDL_GetMouseState(&mousePos.x, &mousePos.y);
    
                cam.lastMousePos = mousePos;
            }
            if (!cam.held) {
                glm::vec2 mousePos{ };
                SDL_GetMouseState(&mousePos.x, &mousePos.y);
                cam.lastMousePos = mousePos;
            }
            cam.held = true;
    
            glm::vec2 mousePos{ };
            SDL_GetMouseState(&mousePos.x, &mousePos.y);
    
            glm::vec2 deltaMousePos = cam.lastMousePos - mousePos;
            deltaMousePos /= 2;
    
            cam.yaw -= deltaMousePos.x * cam.lookSensitivity;
            cam.pitch += deltaMousePos.y * cam.lookSensitivity;
    

            cam.pitch = std::clamp(cam.pitch, -89.9f, 89.9f);

    
            cam.forward.x = cos(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch));
            cam.forward.y = sin(glm::radians(cam.pitch));
            cam.forward.z = sin(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch));
            cam.forward = glm::normalize(cam.forward);
    
            cam.right = glm::normalize(glm::cross(cam.forward, cam.up));
    
            x = cam.position.x;
            y = cam.position.x;
            pitch = cam.pitch;
            yaw = cam.yaw;

            shaderProgram->use();
            cam.lastMousePos = mousePos;


            // for (auto& ent : ecs->get_entities<Sprite>()) {
            //     auto s = ecs->get_component<Sprite>(ent);
            //     s.draw();
            // }
        // });
        } else {
            cam.held = false;
        }
        glm::mat4 model{1.0f};
            model = glm::translate(model, glm::vec3{0.0f, -0.3f, 3.0f});

            model = glm::rotate(model, glm::radians(-90.0f),
                                glm::vec3{1.0f, 0.0f, 0.0f});
            model = glm::rotate(model, glm::radians(-135.0f),
                                glm::vec3{0.0f, 0.0f, 1.0f});

            glm::mat4 projection =
                glm::perspective(glm::radians(60.0f),
                                (float)WIDTH / (float)HEIGHT, 0.01f, 1000.0f);

            glm::mat4 mvp = projection * cam.view_matrix() * model;

            shaderProgram->set_uniform("mvp", mvp);

    }
};

class MeshSystem : public System {
    public:
    void update(ECSController& world) override {
        for (auto& cam_ent : world.get_entities<Mesh>()) {
            // world.get_component<Mesh>(cam_ent).draw();
        }
    }
};

class SpriteSystem : public System {
    public:
    void update(ECSController& world) override {
        for (auto& cam_ent : world.get_entities<Sprite>()) {
            world.get_component<Sprite>(cam_ent).draw();
        }
    }
};
