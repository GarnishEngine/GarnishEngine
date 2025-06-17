cmake_minimum_required(VERSION 3.22)
project(MultiCamTestApp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(MultiCamTestApp multi_cam_test_app.cc)

target_link_libraries(MultiCamTestApp PRIVATE GarnishEngine imgui)
