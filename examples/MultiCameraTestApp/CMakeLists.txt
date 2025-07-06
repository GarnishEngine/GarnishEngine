cmake_minimum_required(VERSION 3.22)
project(MultiCamTestApp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SRC utility/*.cc)
add_executable(MultiCamTestApp multi_cam_test_app.cc ${SRC})

target_link_libraries(MultiCamTestApp PRIVATE GarnishEngine imgui)

target_include_directories(MultiCamTestApp PUBLIC utility)