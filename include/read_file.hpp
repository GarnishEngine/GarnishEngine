#pragma once
#include <fstream>
#include <ios>
#include <string>
#include <vector>

namespace garnish {
inline std::vector<char> read_file(const std::string& path) {
    std::ifstream file{path, std::ios::ate | std::ios::binary};

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open: " + path);
    }

    auto fileSize = static_cast<std::streamsize>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    if (buffer.size() & 3) buffer.resize((buffer.size() + 3) & ~3);

    return buffer;
}
}  // namespace garnish
