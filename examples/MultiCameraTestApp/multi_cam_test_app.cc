#include "garnish_app.hpp"

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

const int xSize{ 100 };
const int ySize{ 100 };

class FramebufferCam : public garnish::Camera {
public:
    FramebufferCam() {
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

        glGenRenderbuffers(1, &renderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, xSize, ySize);

        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderbuffer);

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, xSize, ySize, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texture, 0);

        if (!glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
            std::cout << "ERROR: Framebuffer failed to be created" << std::endl;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // TODO delete copy constructor and assignment operator

    ~FramebufferCam() {
        glDeleteRenderbuffers(1, &renderbuffer);
        glDeleteTextures(1, &texture);
        glDeleteFramebuffers(1, &framebuffer);
    }

    unsigned int framebuffer;
    unsigned int renderbuffer;
    unsigned int texture;
};

// class CameraSystem : public System {
// public:
//     std::unique_ptr<ShaderProgram> shaderProgram;
//     int32_t WIDTH = 600;
//     int32_t HEIGHT = 800;

//     CameraSystem() {
//         shaderProgram = std::make_unique<ShaderProgram>("shaders/shader.vert", "shaders/shader.frag");

//     }
//     void update(ECSController& world) override {
//         auto cam_ent = world.get_entities<garnish::Camera>()[0]; // TODO this is really janky, need to do something about the camera
//         auto& cam = world.get_component<garnish::Camera>(cam_ent);
//         if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_W]) {
//             cam.position += cam.forward * cam.movementSpeed;
//         }
//         if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_S]) {
//             cam.position -= cam.forward * cam.movementSpeed;
//         }
//         if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_D]) {
//             cam.position += cam.right * cam.movementSpeed;
//         }
//         if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_A]) {
//             cam.position -= cam.right * cam.movementSpeed;
//         }
//         if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_SPACE]) {
//             cam.position += cam.up * cam.movementSpeed;
//         }
//         if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LSHIFT]) {
//             cam.position -= cam.up * cam.movementSpeed;
//         }
//         if (SDL_GetMouseState(nullptr, nullptr) == SDL_BUTTON_LEFT) {
//             if (cam.lastMousePos == glm::vec2(std::numeric_limits<float>::max())) {
//                 glm::vec2 mousePos{ };
//                 SDL_GetMouseState(&mousePos.x, &mousePos.y);
    
//                 cam.lastMousePos = mousePos;
//             }
//             if (!cam.held) {
//                 glm::vec2 mousePos{ };
//                 SDL_GetMouseState(&mousePos.x, &mousePos.y);
//                 cam.lastMousePos = mousePos;
//             }
//             cam.held = true;
    
//             glm::vec2 mousePos{ };
//             SDL_GetMouseState(&mousePos.x, &mousePos.y);
    
//             glm::vec2 deltaMousePos = cam.lastMousePos - mousePos;
//             deltaMousePos /= 2;
    
//             cam.yaw -= deltaMousePos.x * cam.lookSensitivity;
//             cam.pitch += deltaMousePos.y * cam.lookSensitivity;
    

//             cam.pitch = std::clamp(cam.pitch, -89.9f, 89.9f);

    
//             cam.forward.x = cos(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch));
//             cam.forward.y = sin(glm::radians(cam.pitch));
//             cam.forward.z = sin(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch));
//             cam.forward = glm::normalize(cam.forward);
    
//             cam.right = glm::normalize(glm::cross(cam.forward, cam.up));
    
//             x = cam.position.x;
//             y = cam.position.x;
//             pitch = cam.pitch;
//             yaw = cam.yaw;

//             shaderProgram->use();
//             cam.lastMousePos = mousePos;


//             // for (auto& ent : ecs->get_entities<Sprite>()) {
//             //     auto s = ecs->get_component<Sprite>(ent);
//             //     s.draw();
//             // }
//         // });
//         } else {
//             cam.held = false;
//         }
//         glm::mat4 model{1.0f};
//             model = glm::translate(model, glm::vec3{0.0f, -0.3f, 3.0f});

//             model = glm::rotate(model, glm::radians(-90.0f),
//                                 glm::vec3{1.0f, 0.0f, 0.0f});
//             model = glm::rotate(model, glm::radians(-135.0f),
//                                 glm::vec3{0.0f, 0.0f, 1.0f});

//             glm::mat4 projection =
//                 glm::perspective(glm::radians(60.0f),
//                                 (float)WIDTH / (float)HEIGHT, 0.01f, 1000.0f);

//             glm::mat4 mvp = projection * cam.view_matrix() * model;

//             shaderProgram->set_uniform("mvp", mvp);

//     }
// };

class MeshSystem : public System {
    public:
    void update(ECSController& world) override {
        for (auto& cam_ent : world.get_entities<Mesh>()) {
            world.get_component<Mesh>(cam_ent).draw();
        }
    }
};

std::unique_ptr<ShaderProgram> shaderProgram;

class RenderingSystem : public System {
public:
    void update(ECSController& world) override {
        int i = 1;
        for (auto& c : world.get_entities<FramebufferCam>()) {
            ImGui::Begin(std::string{ "Camera #" + i }.c_str());
            
            auto& cam = world.get_component<FramebufferCam>(c);

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
                                    (float)xSize / (float)ySize, 0.01f, 1000.0f);

                glm::mat4 mvp = projection * cam.view_matrix() * model;

                shaderProgram->set_uniform("mvp", mvp);


            glBindFramebuffer(GL_FRAMEBUFFER, cam.framebuffer);

            // Render
            for (auto& cam_ent : world.get_entities<Mesh>()) {
                world.get_component<Mesh>(cam_ent).draw();
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            ImGui::Image((ImTextureID)cam.texture, ImVec2{ (float)xSize, (float)ySize });

            ImGui::End();
            ++i;
        }
    }
};

int main() {
    garnish::App app{ };

    shaderProgram = std::make_unique<ShaderProgram>("shaders/shader.vert", "shaders/shader.frag");

    auto r = app.ecsController.register_system<RenderingSystem>(0);
    auto m = app.ecsController.register_system<MeshSystem>(0);

    app.ecsController.register_component<FramebufferCam>();

    auto camera1_entity = app.ecsController.create_entity_with_components(FramebufferCam());
    auto camera2_entity = app.ecsController.create_entity_with_components(FramebufferCam());

    auto vikingRoom = app.ecsController.create_entity();

    garnish::Mesh gMesh;
    garnish::g_texture texture;
    texture.load_texture("Textures/viking_room.png");

    gMesh.load_mesh("Models/viking_room.obj");
    gMesh.setup_mesh();
    gMesh.load_texture(texture);

    app.ecsController.add_component(vikingRoom, gMesh);

    app.run();
}
