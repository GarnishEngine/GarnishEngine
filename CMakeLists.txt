cmake_minimum_required(VERSION 3.22)
project(GarnishEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)





# Settings
option(_VULKAN_RENDERING "Use Vulkan" OFF)
option(_OPENGL_RENDERING "Use OpenGL" ON)
option(_BUILD_EXAMPLES "Build Examples" ON)


# Ensure Vulkan and OpenGL are mutually exclusive
if(_VULKAN_RENDERING)   
    option(_OPENGL_RENDERING "Use OpenGL" OFF)
else()
    option(_OPENGL_RENDERING "Use OpenGL" ON)
endif(_VULKAN_RENDERING)

if (_OPENGL_RENDERING)
    option(_VULKAN_RENDERING "Use Vulkan" OFF)
else()
    option(_VULKAN_RENDERING "Use Vulkan" ON)
endif(_OPENGL_RENDERING)

file(GLOB_RECURSE SRC src/*.cc)
add_library(GarnishEngine STATIC ${SRC})

# Act on Graphics API choice
if (_VULKAN_RENDERING)
    find_package(Vulkan REQUIRED)
    target_link_libraries(GarnishEngine Vulkan::Vulkan)
endif(_VULKAN_RENDERING)

if (_OPENGL_RENDERING)
    find_package(GLEW REQUIRED)
    find_package(OpenGL REQUIRED)
    if (APPLE)
    target_link_libraries(GarnishEngine GLEW::GLEW "-framework OpenGL")
    else()
    target_link_libraries(GarnishEngine GLEW::GLEW GL)
    endif()
endif(_OPENGL_RENDERING)



# 3rdParty libraries
add_subdirectory(3rdParty/SDL EXCLUDE_FROM_ALL)
add_subdirectory(3rdParty/glm EXCLUDE_FROM_ALL)
add_subdirectory(3rdParty/tinyobjloader EXCLUDE_FROM_ALL)

target_include_directories(GarnishEngine PUBLIC 3rdParty/SDL)
target_include_directories(GarnishEngine PUBLIC 3rdParty/glm)
target_include_directories(GarnishEngine PUBLIC 3rdParty/tinyobjloader)
target_include_directories(GarnishEngine PUBLIC 3rdParty/stb)

target_link_libraries(GarnishEngine SDL3::SDL3 glm::glm)


# Building examples:
if (_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif(_BUILD_EXAMPLES)