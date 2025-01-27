#include "garnish_app.hpp"
#include "Rendering/garnish_mesh.hpp"
#include "Utility/garnish_debug.hpp"
#include "garnish_window.hpp"
#include <chrono>
#include <memory>

typedef std::chrono::duration<float> fsec;
const int32_t FRAME_RATE = 60;

namespace garnish {
    GarnishApp::GarnishApp(int32_t w, int32_t h) : WIDTH(w), HEIGHT(h), garnishWindow(w, h, "hello window") {}
    GarnishApp::GarnishApp() : WIDTH(800), HEIGHT(600), garnishWindow(800, 600, "hello window") {}

    bool GarnishApp::handle_poll_event() {
        GarnishEvent event{};
        if (!event.state)
            return false;
        if (event.event.window.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            garnishWindow.shouldClose = true;
        }
        if (event.event.window.type == SDL_EVENT_WINDOW_RESIZED) {
            garnishWindow.pairWindowSize(&WIDTH, &HEIGHT);
            glViewport(0, 0, WIDTH, HEIGHT);
        }  

        for (const auto &entity : entities) {
            entity->update(event);
        }

        return true;
    }
    
    void GarnishApp::handle_all_events() {
        while (handle_poll_event()) {}
    }

    // struct vertex {
    //     glm::vec3 pos{ };
    //     glm::vec3 color{ };
    // };

    void GarnishApp::run() {
        if (glewInit() != GLEW_OK) {
            throw std::runtime_error("GLEW failed to initialize");
        }
        std::shared_ptr<Camera> cam = std::make_shared<Camera>();
        entities.push_back(cam);

        ShaderProgram shaderProgram{ "shaders/shader.vert", "shaders/shader.frag" };

        std::vector<vertex> cubeVertices{ 
            vertex{ glm::vec3{ -0.5f, -0.5f,  0.5f }, glm::vec3{ 0.0f, 1.0f, 0.0f } },
            vertex{ glm::vec3{ -0.5f,  0.5f,  0.5f }, glm::vec3{ 1.0f, 0.0f, 0.0f } },
            vertex{ glm::vec3{  0.5f,  0.5f,  0.5f }, glm::vec3{ 1.0f, 1.0f, 1.0f } },
            vertex{ glm::vec3{  0.5f, -0.5f,  0.5f }, glm::vec3{ 0.0f, 0.0f, 1.0f } },

            vertex{ glm::vec3{  0.5f, -0.5f,  0.5f }, glm::vec3{ 0.0f, 0.0f, 1.0f } },
            vertex{ glm::vec3{  0.5f,  0.5f,  0.5f }, glm::vec3{ 1.0f, 1.0f, 1.0f } },
            vertex{ glm::vec3{  0.5f,  0.5f, -0.5f }, glm::vec3{ 1.0f, 1.0f, 0.0f } },
            vertex{ glm::vec3{  0.5f, -0.5f, -0.5f }, glm::vec3{ 0.0f, 1.0f, 1.0f } },

            vertex{ glm::vec3{ -0.5f, -0.5f, -0.5f }, glm::vec3{ 0.0f, 0.0f, 0.0f } },
            vertex{ glm::vec3{ -0.5f,  0.5f, -0.5f }, glm::vec3{ 1.0f, 0.0f, 1.0f } },
            vertex{ glm::vec3{ -0.5f,  0.5f,  0.5f }, glm::vec3{ 1.0f, 0.0f, 0.0f } },
            vertex{ glm::vec3{ -0.5f, -0.5f,  0.5f }, glm::vec3{ 0.0f, 1.0f, 0.0f } },

            vertex{ glm::vec3{  0.5f, -0.5f, -0.5f }, glm::vec3{ 0.0f, 1.0f, 1.0f } },
            vertex{ glm::vec3{  0.5f,  0.5f, -0.5f }, glm::vec3{ 1.0f, 1.0f, 0.0f } },
            vertex{ glm::vec3{ -0.5f,  0.5f, -0.5f }, glm::vec3{ 1.0f, 0.0f, 1.0f } },
            vertex{ glm::vec3{ -0.5f, -0.5f, -0.5f }, glm::vec3{ 0.0f, 0.0f, 0.0f } },

            vertex{ glm::vec3{ -0.5f,  0.5f,  0.5f }, glm::vec3{ 1.0f, 0.0f, 0.0f } },
            vertex{ glm::vec3{ -0.5f,  0.5f, -0.5f }, glm::vec3{ 1.0f, 0.0f, 1.0f } },
            vertex{ glm::vec3{  0.5f,  0.5f, -0.5f }, glm::vec3{ 1.0f, 1.0f, 0.0f } },
            vertex{ glm::vec3{  0.5f,  0.5f,  0.5f }, glm::vec3{ 1.0f, 1.0f, 1.0f } },

            vertex{ glm::vec3{ -0.5f, -0.5f, -0.5f }, glm::vec3{ 0.0f, 0.0f, 0.0f } },
            vertex{ glm::vec3{ -0.5f, -0.5f,  0.5f }, glm::vec3{ 0.0f, 1.0f, 0.0f } },
            vertex{ glm::vec3{  0.5f, -0.5f,  0.5f }, glm::vec3{ 0.0f, 0.0f, 1.0f } },
            vertex{ glm::vec3{  0.5f, -0.5f, -0.5f }, glm::vec3{ 0.0f, 1.0f, 1.0f } }
        };

        std::vector<float> vertices{ };
        for (auto& vert : cubeVertices) {
            vertices.push_back(vert.pos.x);
            vertices.push_back(vert.pos.y);
            vertices.push_back(vert.pos.z);

            vertices.push_back(vert.color.x);
            vertices.push_back(vert.color.y);
            vertices.push_back(vert.color.z);
        }



        std::vector<unsigned int> indices = {
            0, 1, 2,
            0, 2, 3,

            4, 5, 6,
            4, 6, 7,

            8, 9, 10,
            8, 10, 11,

            12, 13, 14,
            12, 14, 15,

            16, 17, 18,
            16, 18, 19,

            20, 21, 22,
            20, 22, 23,
        };

        // unsigned int VAO;
        // glGenVertexArrays(1, &VAO);
        // glBindVertexArray(VAO);

        // VertexBufferObject VBO{vertices};
        // ElementBufferObject EBO{indices};

        // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        // glEnableVertexAttribArray(0);

        // glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        // glEnableVertexAttribArray(1);
        GarnishMesh gMesh;
        gMesh.loadModel("Models/viking_room.obj");
        gMesh.setupMesh();
        gMesh.loadTexture("Textures/viking_room.png");

            SDL_Event event;
        tp end_time = hrclock::now();
        
        while (!shouldClose()) {
            
            tp start_time = hrclock::now();

            int32_t dt = duration_cast<ms>(start_time - end_time).count();

            if (dt > 1000/FRAME_RATE) {
                    
                handle_all_events();
                glViewport(0, 0, WIDTH, HEIGHT);
                glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                shaderProgram.Use();

                glm::mat4 model{1.0f};
                glm::mat4 projection = glm::perspective(
                    glm::radians(60.0f), (float)WIDTH / (float)HEIGHT, 0.01f,
                    1000.0f);

                glm::mat4 mvp = projection * cam->ViewMatrix() * model;

                shaderProgram.SetUniform("mvp", mvp);

                gMesh.draw();
                garnishWindow.SwapWindow();

                end_time = start_time;
                // TODO: make so things work with dt, probably should move it to class member or something

                // std::cerr << "frame: " << ++frame_num
                //             << " ms_elapsed_since_last: " << dt << '\n';
            }
        }

        // gMesh.deleteVertexArray();
        // glDeleteVertexArrays(1, &VAO);
    }
}
