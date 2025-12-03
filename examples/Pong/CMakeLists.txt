cmake_minimum_required(VERSION 3.22)
project(Pong LANGUAGES CXX)

add_executable(Pong pong.cc)

target_link_libraries(Pong PRIVATE GarnishEngine imgui)
