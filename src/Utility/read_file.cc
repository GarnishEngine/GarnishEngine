#include "read_file.hpp"

#include <fstream>
#include <ios>

namespace garnish {
std::vector<char> read_file(const std::string& path) {
    std::ifstream file{path, std::ios::ate | std::ios::binary};

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open: " + path);
    }

    auto fileSize = static_cast<std::streamsize>(file.tellg());
    std::vector<char> buffer(fileSize + 1);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    buffer[fileSize] = '\0';

    file.close();
    return buffer;
}
}  // namespace garnish