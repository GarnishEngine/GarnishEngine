cmake_minimum_required(VERSION 3.22)
project(TestApp LANGUAGES CXX)

add_executable(TestApp test_app.cc)

target_link_libraries(TestApp PRIVATE GarnishEngine imgui)