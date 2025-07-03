cmake_minimum_required(VERSION 3.22)

project(GarnishEngine)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(GARNISH_PRIVATE_LIBS "") 
# Settings

file(GLOB_RECURSE ENGINE_SRC
     src/*.cc
     src/*.cpp)
list(FILTER ENGINE_SRC EXCLUDE REGEX ".*/Rendering/[^/]+/.*")

option(_BUILD_EXAMPLES "Build Examples" ON)

add_library(GarnishEngine STATIC)

# 3rdParty libraries
add_subdirectory(3rdParty/SDL           EXCLUDE_FROM_ALL)
add_subdirectory(3rdParty/glm           EXCLUDE_FROM_ALL)
add_subdirectory(3rdParty/tinyobjloader EXCLUDE_FROM_ALL)
add_subdirectory(GarnishECS             EXCLUDE_FROM_ALL)

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

target_include_directories(GarnishEngine
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include     
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Rendering     
    3rdParty/tinyobjloader
    3rdParty/stb
    GarnishECS/include
)

set(RENDER_BACKENDS "OpenGL" "Vulkan"              
    CACHE STRING "Semicolon-separated list of rendering back-ends to build")

set_property(CACHE RENDER_BACKENDS PROPERTY STRINGS
             "OpenGL" "Vulkan")

macro(backends_contains name result)
    list(FIND RENDER_BACKENDS "${name}" _idx)
    if(_idx EQUAL -1)
        set(${result} FALSE)
    else()
        set(${result} TRUE)
    endif()
endmacro()

backends_contains("OpenGL" USE_OPENGL)
if(USE_OPENGL)
    file(GLOB_RECURSE OPENGL_SRC  src/Rendering/OpenGL/*.cc)
    list(APPEND ENGINE_SRC ${OPENGL_SRC})

    find_package(GLEW REQUIRED)
    find_package(OpenGL REQUIRED)

    list(APPEND GARNISH_PRIVATE_LIBS GLEW::GLEW)
    if(APPLE)
        list(APPEND GARNISH_PRIVATE_LIBS "-framework OpenGL")
    else()
        list(APPEND GARNISH_PRIVATE_LIBS GL)
    endif()

    target_include_directories(GarnishEngine PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Rendering/OpenGL)

    target_compile_definitions(GarnishEngine PUBLIC _OPENGL_RENDERING)
endif()


backends_contains("Vulkan" USE_VULKAN)
if(USE_VULKAN)
    file(GLOB_RECURSE VULKAN_SRC src/Rendering/VulkanBackend/*.cc)
    list(APPEND ENGINE_SRC ${VULKAN_SRC})
    find_package(Vulkan REQUIRED)
    add_subdirectory(3rdParty/VulkanMemoryAllocator EXCLUDE_FROM_ALL)

    list(APPEND GARNISH_PRIVATE_LIBS
        Vulkan::Vulkan
        GPUOpen::VulkanMemoryAllocator)

    target_include_directories(GarnishEngine PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Rendering/VulkanBackend)

    target_compile_definitions(GarnishEngine PUBLIC _VULKAN_RENDERING)
endif()

if(NOT USE_OPENGL AND NOT USE_VULKAN)
    message(FATAL_ERROR "RENDER_BACKENDS must contain at least one of: OpenGL, Vulkan")
endif()

target_sources(GarnishEngine PRIVATE ${ENGINE_SRC})
target_compile_definitions(GarnishEngine PUBLIC
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
target_link_libraries(GarnishEngine PRIVATE ${GARNISH_PRIVATE_LIBS})
# target_sources(GarnishEngine PRIVATE "src/shared.cc")

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