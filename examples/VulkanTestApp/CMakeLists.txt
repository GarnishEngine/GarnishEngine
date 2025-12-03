cmake_minimum_required(VERSION 3.22)
project(VulkanTestApp LANGUAGES CXX)

add_executable(VulkanTestApp test_app.cc)

target_link_libraries(VulkanTestApp PRIVATE GarnishEngine imgui)