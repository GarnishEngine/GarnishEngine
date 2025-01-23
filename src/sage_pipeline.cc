#include "sage_pipeline.hpp"
#include <cstddef>
#include <stdexcept>


namespace garnish {
    SagePipeline::SagePipeline(const std::string& vertFilepath, const std::string& fragFilepath) {
        CreateGraphicsPipeline(vertFilepath, fragFilepath);
    }

    std::vector<char> SagePipeline::readFile(const std::string &filepath) {
        std::ifstream file{filepath, std::ios::ate | std::ios::binary};

        if (!file.is_open()) {
            throw std::runtime_error("Failed to open: " + filepath);
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();
        return buffer;
    }

    void SagePipeline::CreateGraphicsPipeline(
        const std::string &vertFilepath, const std::string &fragFilepath) {
            auto vertCode = readFile(vertFilepath);
            auto fragCode = readFile(fragFilepath);

            std::cout << "Vert code size: " << vertCode.size() << '\n';
            std::cout << "Frag code size: " << vertCode.size() << std::endl;
    }   
}
