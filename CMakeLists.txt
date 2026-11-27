cmake_minimum_required(VERSION 3.16)
project(MySunSimulation LANGUAGES CXX)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Vulkan package
# Find Vulkan package
find_package(Vulkan REQUIRED)

# Add GLFW as submodule (built locally)
# add_subdirectory removed - using FetchContent


# Find GLFW for window/context creation
include(FetchContent)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)


# Add executable
add_executable(MySunSimulation
    src/main.cpp
)

# Include shader directory and compile shaders automatically
set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
find_program(GLSLC_EXECUTABLE glslc)
if (GLSLC_EXECUTABLE)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/vert.spv
        COMMAND ${GLSLC_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/shaders/sun.vert -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/vert.spv
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/shaders/vert.spv ${CMAKE_CURRENT_SOURCE_DIR}/shaders/vert.spv
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/sun.vert
        COMMENT "Compiling sun.vert to SPIR-V"
    )
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/frag.spv
        COMMAND ${GLSLC_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/shaders/sun.frag -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/frag.spv
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/shaders/frag.spv ${CMAKE_CURRENT_SOURCE_DIR}/shaders/frag.spv
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/sun.frag
        COMMENT "Compiling sun.frag to SPIR-V"
    )
    add_custom_target(CompileShaders ALL
        DEPENDS
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/vert.spv
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/frag.spv
    )
    add_dependencies(MySunSimulation CompileShaders)
endif()

# Link libraries
target_link_libraries(MySunSimulation PRIVATE Vulkan::Vulkan glfw)

# On Windows need to link additional libraries
if (WIN32)
    target_link_libraries(MySunSimulation PRIVATE ${Vulkan_LIBRARY})
endif()
