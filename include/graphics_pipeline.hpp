#pragma once
#include <iostream>
#include <string>
namespace garnish {
class GraphicsPipeline {
   public:
    GraphicsPipeline(
        const std::string& vert_file,
        const std::string& frag_file
    );

   private:
    void create_graphics_pipeline(
        const std::string& vert_file,
        const std::string& frag_file
    );
};
}  // namespace garnish
