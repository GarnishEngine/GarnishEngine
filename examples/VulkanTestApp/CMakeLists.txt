cmake_minimum_required(VERSION 3.22)
project(VulkanTestApp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(VulkanTestApp test_app.cc)

target_include_directories(VulkanTestApp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

target_link_libraries(VulkanTestApp PRIVATE GarnishEngine imgui -fsanitize=address,undefined)
target_compile_options(VulkanTestApp PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)