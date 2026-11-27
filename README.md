# My Sun Simulation

A C++ Vulkan project that renders a dynamic, realistic Sun with flares, solar storms, and sunspots.

## Features
- Procedurally generated UV sphere representing the Sun
- Emissive shading for bright solar surface
- Placeholder for flare, storm, and sunspot effects (to be implemented in shaders)
- Build system using CMake
- Cross‑platform (Windows/Linux/macOS) with GLFW window handling

## Prerequisites
- C++17 compatible compiler (gcc, clang, MSVC)
- Vulkan SDK (https://vulkan.lunarg.com/)
- GLFW 3.3+ (`sudo apt-get install libglfw3-dev` on Ubuntu)
- CMake 3.16+
- glslc (shader compiler) to compile GLSL shaders to SPIR‑V

## Build Instructions
```bash
mkdir build && cd build
cmake ..
make
./MySunSimulation
```

## Shaders
The project expects compiled SPIR‑V shaders located in `shaders/`:
- `vert.spv` – vertex shader (compile `vert.glsl`)
- `frag.spv` – fragment shader (compile `frag.glsl`)

Compile them with:
```bash
glslc ../shaders/vert.glsl -o vert.spv

glslc ../shaders/frag.glsl -o frag.spv
```

## License
MIT License – see LICENSE file.
