#pragma once

#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>

namespace garnish {
    class GraphicsPipeline{
      public:
        GraphicsPipeline(const std::string &vertFilepath, const std::string &fragFilepath);
                    
      private:
        static std::vector<char> readFile(const std::string &filepath);
        void CreateGraphicsPipeline(const std::string &vertFilepath, const std::string &fragFilepath);
    };
}
