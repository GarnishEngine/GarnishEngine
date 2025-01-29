cmake_minimum_required(VERSION 3.22)
project(TestApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(TestApp test_app.cc)

target_include_directories(TestApp PUBLIC ../../src)

target_include_directories(TestApp PUBLIC ../../3rdParty/SDL)
target_include_directories(TestApp PUBLIC ../../3rdParty/glm)
target_include_directories(TestApp PUBLIC ../../3rdParty/tinyobjloader)
target_include_directories(TestApp PUBLIC ../../3rdParty/stb)

target_link_libraries(TestApp GarnishEngine)
