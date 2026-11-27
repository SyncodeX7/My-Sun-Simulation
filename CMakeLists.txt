cmake_minimum_required(VERSION 3.16)
project(MySunSimulation LANGUAGES CXX)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Vulkan package
find_package(Vulkan REQUIRED)

# Find GLFW for window/context creation
find_package(glfw3 3.3 REQUIRED)

# Add executable
add_executable(MySunSimulation
    src/main.cpp
)

# Include shader directory (for custom command to compile shaders if needed)
set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)

# Link libraries
target_link_libraries(MySunSimulation PRIVATE Vulkan::Vulkan glfw)

# On Windows need to link additional libraries
if (WIN32)
    target_link_libraries(MySunSimulation PRIVATE ${Vulkan_LIBRARY})
endif()
