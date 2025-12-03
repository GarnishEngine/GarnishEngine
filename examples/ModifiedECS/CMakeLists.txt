cmake_minimum_required(VERSION 3.22)
project(ModifiedECS LANGUAGES CXX)

add_executable(ModifiedECS modifiedECS.cc)

target_link_libraries(ModifiedECS PRIVATE GarnishEngine imgui)