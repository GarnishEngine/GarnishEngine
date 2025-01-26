cmake_minimum_required(VERSION 3.22)
project(GarnishEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")


option(_VULKAN_RENDERING "Use Vulkan" OFF)
option(_OPENGL_RENDERING "Use OpenGL" ON)
option(_USE_OWN_SDL "Use your own SDL installation" OFF)
option(_USE_OWN_GLM "Use your own GLM installation" OFF)
option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)

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

file(GLOB_RECURSE INC include/*.h include/*.hpp)
file(GLOB_RECURSE SRC src/*.cc)

if(BUILD_SHARED_LIBS)
add_library(GarnishEngine SHARED ${INC} ${SRC})
set_property(TARGET GarnishEngine PROPERTY POSITION_INDEPENDENT_CODE ON)
else()
add_library(GarnishEngine STATIC ${INC} ${SRC})
endif()

add_library(garnish::GarnishEngine ALIAS GarnishEngine)

target_include_directories(GarnishEngine PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
list(APPEND GARNISH_TARGETS GarnishEngine)

add_executable(GarnishApp main.cc ${SOURCE_FILES} )


if(_VULKAN_RENDERING)
target_link_libraries(GarnishApp GarnishEngine Vulkan::Vulkan SDL3::SDL3)
else()
target_link_libraries(GarnishApp GarnishEngine ${OPENGL_LIBRARIES} SDL3::SDL3 GLEW::GLEW GL glm::glm)
endif(_VULKAN_RENDERING)

install(TARGETS ${GARNISH_TARGETS}
        EXPORT garnish-export
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(DIRECTORY "${PROJECT_SOURCE_DIR}/src/"
        DESTINATION include ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "*.inl")

