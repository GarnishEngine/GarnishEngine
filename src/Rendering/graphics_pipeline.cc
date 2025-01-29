#include "graphics_pipeline.hpp"
#include <cstddef>
#include <stdexcept>

#include "../Utility/read_file.hpp"

namespace garnish {
    GraphicsPipeline::GraphicsPipeline(const std::string& vertFilepath, const std::string& fragFilepath) {
        CreateGraphicsPipeline(vertFilepath, fragFilepath);
    }

    void GraphicsPipeline::CreateGraphicsPipeline(
        const std::string &vertFilepath, const std::string &fragFilepath) {
            auto vertCode = ReadFile(vertFilepath);
            auto fragCode = ReadFile(fragFilepath);

            std::cout << "Vert code size: " << vertCode.size() << '\n';
            std::cout << "Frag code size: " << vertCode.size() << std::endl;
    }   
}
