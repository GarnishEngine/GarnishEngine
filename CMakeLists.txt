cmake_minimum_required(VERSION 3.22)
project(GarnishEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)

add_subdirectory(3rdParty/SDL EXCLUDE_FROM_ALL)

add_executable(${PROJECT_NAME} main.cc
                    src/carrot_window.cc 
                    src/garnish_app.cc )

target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan SDL3::SDL3)
