cmake_minimum_required(VERSION 3.22)
project(Pong)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(Pong pong.cc)

target_include_directories(Pong PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

target_link_libraries(Pong PRIVATE GarnishEngine imgui)
