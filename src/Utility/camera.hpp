#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

namespace garnish {
    struct Camera {
        glm::vec3 position{ 0.0f, 0.0f, 5.0f };

        glm::vec3 up{ 0.0f, 1.0f, 0.0f };
        glm::vec3 forward{ 0.0f, 0.0f, -1.0f };
        glm::vec3 right{ 1.0f, 0.0f, 0.0f };

        // TODO Camera movement/looking around
        // float yaw{ 0.0f };
        // float pitch{ 0.0f };

        glm::mat4 ViewMatrix();
    };
}