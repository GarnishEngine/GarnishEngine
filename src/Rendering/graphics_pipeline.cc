#include "graphics_pipeline.hpp"
#include <cstddef>
#include <stdexcept>

#include "read_file.hpp"

namespace garnish {
    GraphicsPipeline::GraphicsPipeline(const std::string& vert_file, const std::string& frag_file) {
        create_graphics_pipeline(vert_file, frag_file);
    }

    void GraphicsPipeline::create_graphics_pipeline(
        const std::string &vert_file, const std::string &frag_file) {
            auto vert_code = read_file(vert_file);
            auto frag_code = read_file(frag_file);

            std::cout << "Vert code size: " << vert_code.size() << '\n';
            std::cout << "Frag code size: " << vert_code.size() << std::endl;
    }   
}
