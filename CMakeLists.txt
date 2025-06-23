cmake_minimum_required(VERSION 3.22)

project(GarnishEngine)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(GARNISH_PRIVATE_LIBS "") 
# Settings

option(_BUILD_EXAMPLES "Build Examples" ON)


# 3rdParty libraries
add_subdirectory(3rdParty/SDL EXCLUDE_FROM_ALL)
add_subdirectory(3rdParty/glm EXCLUDE_FROM_ALL)
add_subdirectory(3rdParty/tinyobjloader EXCLUDE_FROM_ALL)
add_subdirectory(GarnishECS EXCLUDE_FROM_ALL)

file(GLOB IMGUI_SOURCES
        "3rdParty/imgui/*.h"
        "3rdParty/imgui/*.cpp"
        "3rdParty/imgui/backends/imgui_impl_opengl3.cpp"
        "3rdParty/imgui/backends/imgui_impl_sdl3.cpp")
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC 3rdParty/imgui
                                        3rdParty/imgui/backends)
target_link_libraries(imgui PUBLIC SDL3::SDL3 OpenGL::GL GLEW::GLEW)
list(APPEND GARNISH_PRIVATE_LIBS SDL3::SDL3 glm::glm GarnishECS imgui)

set(RENDER_API "OpenGL" CACHE STRING "Choose rendering backend")
set_property(CACHE RENDER_API PROPERTY STRINGS OpenGL Vulkan)
option(_VULKAN_RENDERING "Use Vulkan" OFF)
option(_OPENGL_RENDERING "Use OpenGL" ON)
if(RENDER_API STREQUAL "OpenGL")
    set(_OPENGL_RENDERING ON)
    set(_VULKAN_RENDERING OFF)

    find_package(GLEW REQUIRED)
    find_package(OpenGL REQUIRED)

    list(APPEND GARNISH_PRIVATE_LIBS GLEW::GLEW)
    if (APPLE)
    list(APPEND GARNISH_PRIVATE_LIBS "-framework OpenGL")
    else()
    list(APPEND GARNISH_PRIVATE_LIBS GL)
    endif()
elseif(RENDER_API STREQUAL "Vulkan")
    set(_OPENGL_RENDERING OFF)
    set(_VULKAN_RENDERING ON)

    find_package(Vulkan REQUIRED)
    add_subdirectory(3rdParty/vk-bootstrap EXCLUDE_FROM_ALL)
    add_subdirectory(3rdParty/VulkanMemoryAllocator EXCLUDE_FROM_ALL)

    list(APPEND GARNISH_PRIVATE_LIBS
        Vulkan::Vulkan
        vk-bootstrap::vk-bootstrap
        GPUOpen::VulkanMemoryAllocator)
else()
    message(FATAL_ERROR "RENDER_API must be OpenGL or Vulkan")
endif()

find_package(Vulkan REQUIRED)
add_subdirectory(3rdParty/vk-bootstrap EXCLUDE_FROM_ALL)
add_subdirectory(3rdParty/VulkanMemoryAllocator EXCLUDE_FROM_ALL)

list(APPEND GARNISH_PRIVATE_LIBS
    Vulkan::Vulkan
    vk-bootstrap::vk-bootstrap
    GPUOpen::VulkanMemoryAllocator)

file(GLOB_RECURSE SRC src/*.cc)
add_library(GarnishEngine STATIC ${SRC})
target_include_directories(GarnishEngine
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include          
    3rdParty/tinyobjloader
    3rdParty/stb
    GarnishECS/include
)

target_link_libraries(GarnishEngine PRIVATE ${GARNISH_PRIVATE_LIBS})

set_target_properties(GarnishEngine PROPERTIES
  PUBLIC_HEADER "include/garnish.hpp"
)

install(TARGETS GarnishEngine
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/GarnishEngine)
    
if (_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif(_BUILD_EXAMPLES)