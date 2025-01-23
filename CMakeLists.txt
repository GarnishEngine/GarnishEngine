cmake_minimum_required(VERSION 3.22)
project(GarnishEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(_VULKAN_RENDERING "Use Vulkan" OFF)
option(_USE_OWN_SDL "Use Vulkan" OFF)


if(_VULKAN_RENDERING)   
    find_package(Vulkan REQUIRED)
else()
    find_package(OpenGL REQUIRED)
    include_directories( ${OPENGL_INCLUDE_DIRS}  )
endif(_VULKAN_RENDERING)

if(_USE_OWN_SDL)
    find_package(SDL3 REQUIRED)
else()
    add_subdirectory(3rdParty/SDL EXCLUDE_FROM_ALL)
endif(_USE_OWN_SDL)

# Glob all C/C++ files in the src directory
file(GLOB SOURCE_FILES src/*.cc)

add_executable(${PROJECT_NAME} main.cc ${SOURCE_FILES})


if(_VULKAN_RENDERING)
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan SDL3::SDL3)
else()
target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} SDL3::SDL3)

endif(_VULKAN_RENDERING)
