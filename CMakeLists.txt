cmake_minimum_required(VERSION 3.22)
project(GarnishEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(_VULKAN_RENDERING "Use Vulkan" OFF)
option(_OPENGL_RENDERING "Use OpenGL" ON)
option(_USE_OWN_SDL "Use your own SDL installation" OFF)
option(_USE_OWN_GLM "Use your own GLM installation" OFF)

if(_VULKAN_RENDERING)   
    find_package(Vulkan REQUIRED)
    option(_OPENGL_RENDERING "Use OpenGL" OFF)
else()
    option(_OPENGL_RENDERING "Use OpenGL" ON)
endif(_VULKAN_RENDERING)

if (_OPENGL_RENDERING)
    find_package(GLEW REQUIRED)
    find_package(OpenGL REQUIRED)
    include_directories( ${OPENGL_INCLUDE_DIRS}  )
endif(_OPENGL_RENDERING)

if(_USE_OWN_SDL)
    find_package(SDL3 REQUIRED)
else()
    add_subdirectory(3rdParty/SDL EXCLUDE_FROM_ALL)
endif(_USE_OWN_SDL)

if(_USE_OWN_GLM)
    find_package(glm REQUIRED)
else()
    add_subdirectory(3rdParty/glm EXCLUDE_FROM_ALL)
endif(_USE_OWN_GLM)

# Glob all C/C++ files in the src directory
file(GLOB SOURCE_FILES 
    src/*.cc 
    src/Utility/*.cc
    src/Utility/OpenGL/*.cc
)

add_executable(${PROJECT_NAME} main.cc ${SOURCE_FILES})

if(_VULKAN_RENDERING)
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan SDL3::SDL3)
else()
target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} SDL3::SDL3 GLEW::GLEW GL glm::glm)
endif(_VULKAN_RENDERING)
