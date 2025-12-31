#pragma once
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <string_view>
#include <vector>

namespace garnish {
[[nodiscard]] inline std::vector<char> read_file(std::string_view path) {
    std::ifstream file{std::filesystem::path{path}, std::ios::ate | std::ios::binary};

    if (!file.is_open()) {
        throw std::runtime_error(std::format("Failed to open: {}", path));
    }

    auto fileSize = static_cast<std::streamsize>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    if (buffer.size() & 3) buffer.resize((buffer.size() + 3) & ~3);
    return buffer;
}
}  // namespace garnish
