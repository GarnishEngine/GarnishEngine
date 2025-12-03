cmake_minimum_required(VERSION 3.22)

project(GarnishEngine LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(ENABLE_SANITIZERS "Enable address/UB sanitizers" OFF)
option(_BUILD_EXAMPLES "Build Examples" ON)

# Select rendering backends (semicolon-separated list)
set(RENDER_BACKENDS "OpenGL;Vulkan" CACHE STRING "Semicolon-separated list of rendering back-ends to build")
set_property(CACHE RENDER_BACKENDS PROPERTY STRINGS "OpenGL;Vulkan")

# Library target
add_library(GarnishEngine STATIC)
# Require C++20
target_compile_features(GarnishEngine PUBLIC cxx_std_20)

# Helper to check backend selection
function(backends_contains name result)
    list(FIND RENDER_BACKENDS "${name}" _idx)
    if(_idx EQUAL -1)
        set(${result} FALSE PARENT_SCOPE)
    else()
        set(${result} TRUE PARENT_SCOPE)
    endif()
endfunction()

backends_contains("OpenGL" USE_OPENGL)
backends_contains("Vulkan" USE_VULKAN)

if(NOT USE_OPENGL AND NOT USE_VULKAN)
    message(FATAL_ERROR "RENDER_BACKENDS must contain at least one of: OpenGL, Vulkan")
endif()

# Third-party deps
add_subdirectory(3rdParty/SDL           EXCLUDE_FROM_ALL)
add_subdirectory(3rdParty/glm           EXCLUDE_FROM_ALL)
add_subdirectory(3rdParty/tinyobjloader EXCLUDE_FROM_ALL)
add_subdirectory(3rdParty/GarnishECS    EXCLUDE_FROM_ALL)

file(GLOB IMGUI_SOURCES
        "3rdParty/imgui/*.h"
        "3rdParty/imgui/*.cpp"
        "3rdParty/imgui/backends/imgui_impl_opengl3.cpp"
        "3rdParty/imgui/backends/imgui_impl_sdl3.cpp")
add_library(imgui STATIC ${IMGUI_SOURCES})

target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/imgui
                                        ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/imgui/backends)
target_link_libraries(imgui PUBLIC SDL3::SDL3 OpenGL::GL GLEW::GLEW)
list(APPEND GARNISH_PRIVATE_LIBS SDL3::SDL3 glm::glm GarnishECS imgui)

target_include_directories(GarnishEngine
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include     
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Rendering     
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/tinyobjloader
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/stb
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/GarnishECS/include
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/glm
)
set(IMGUI_BACKENDS 3rdParty/imgui/backends/imgui_impl_sdl3.cpp)
if(USE_OPENGL)
    list(APPEND IMGUI_BACKENDS 3rdParty/imgui/backends/imgui_impl_opengl3.cpp)
endif()
add_library(imgui STATIC ${IMGUI_CORE} ${IMGUI_BACKENDS})

target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/imgui/backends
)
# Link ImGui to SDL always; GL only when OpenGL backend is enabled
target_link_libraries(imgui PUBLIC SDL3::SDL3)
if(USE_OPENGL)
    find_package(GLEW REQUIRED)
    find_package(OpenGL REQUIRED)
    target_link_libraries(imgui PUBLIC GLEW::GLEW OpenGL::GL)
endif()

# Collect engine sources (exclude backend dirs, add them explicitly below)
file(GLOB_RECURSE ENGINE_SRC CONFIGURE_DEPENDS
    src/*.cc
    src/*.cpp
)
list(FILTER ENGINE_SRC EXCLUDE REGEX ".*/Rendering/[^/]+/.*")

# Backend sources and libs
set(GARNISH_PRIVATE_LIBS SDL3::SDL3 glm::glm GarnishECS imgui)

if(USE_OPENGL)
    file(GLOB_RECURSE OPENGL_SRC CONFIGURE_DEPENDS src/Rendering/OpenGL/*.cc)
    list(APPEND ENGINE_SRC ${OPENGL_SRC})

    # Link GL on each platform
    list(APPEND GARNISH_PRIVATE_LIBS GLEW::GLEW)
    if(APPLE)
        list(APPEND GARNISH_PRIVATE_LIBS "-framework OpenGL")
    else()
        list(APPEND GARNISH_PRIVATE_LIBS GL)
    endif()

    target_include_directories(GarnishEngine PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Rendering/OpenGL
    )
    target_include_directories(GarnishEngine PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Rendering/OpenGL
    )

    target_compile_definitions(GarnishEngine PUBLIC _OPENGL_RENDERING)
endif()

if(USE_VULKAN)
    file(GLOB_RECURSE VULKAN_SRC CONFIGURE_DEPENDS src/Rendering/VulkanBackend/*.cc)
    list(APPEND ENGINE_SRC ${VULKAN_SRC})

    find_package(Vulkan REQUIRED)
    add_subdirectory(3rdParty/VulkanMemoryAllocator EXCLUDE_FROM_ALL)
    list(APPEND GARNISH_PRIVATE_LIBS Vulkan::Vulkan GPUOpen::VulkanMemoryAllocator)

    target_include_directories(GarnishEngine PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Rendering/VulkanBackend
    )
    target_include_directories(GarnishEngine PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Rendering/VulkanBackend
        ${Vulkan_INCLUDE_DIRS}
    )

    target_compile_definitions(GarnishEngine PUBLIC _VULKAN_RENDERING)
endif()

# Public/Private include paths
# Public headers exposed to consumers
target_include_directories(GarnishEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Rendering
    ${CMAKE_CURRENT_SOURCE_DIR}/GarnishECS/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src            # TODO: migrate public headers out of src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Rendering  # TODO: migrate public headers out of src
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/glm
)
# Private includes internal to the engine
target_include_directories(GarnishEngine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/tinyobjloader
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/stb
)

# Sources
target_sources(GarnishEngine PRIVATE ${ENGINE_SRC})

# Global compile defs
target_compile_definitions(GarnishEngine PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)

# Sanitizers
if(ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_library(sanitizers INTERFACE)
    target_compile_options(sanitizers INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(sanitizers INTERFACE -fsanitize=address,undefined)
    target_link_libraries(GarnishEngine PRIVATE sanitizers)
endif()

# Link libs
target_link_libraries(GarnishEngine PRIVATE ${GARNISH_PRIVATE_LIBS})

# Install
set_target_properties(GarnishEngine PROPERTIES
    PUBLIC_HEADER "include/garnish.hpp"
)

install(TARGETS GarnishEngine
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/GarnishEngine
)

# Examples
if(_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()