# My Sun Simulation

A C++ Vulkan project that renders a dynamic, realistic Sun with flares, solar storms, and sunspots.

## Features
- Procedural fullscreen raymarched/screen-space Sun rendering with Vulkan
- Dynamic solar surface granulation and convection cells using 3D noise
- Sunspots with core (umbra) and penumbra structures
- Solar prominences and flares extending near the limb
- Solar storms with turbulent flares
- Coronal glow and atmospheric limb darkening
- Build system using CMake with GLFW 3.4 (via FetchContent) and automatic shader compilation with glslc
- Cross-platform with GLFW window handling

## Prerequisites
- C++17 compatible compiler (gcc, clang, MSVC)
- Vulkan SDK
- GLFW 3.3+ (or automatically fetched via CMake)
- CMake 3.16+
- glslc (shader compiler) to compile GLSL shaders to SPIR-V

## Build Instructions
```bash
mkdir -p build && cd build
cmake ..
cmake --build .
./MySunSimulation
```

## Shaders
The project uses procedural GLSL shaders located in `shaders/`:
- `sun.vert` – Fullscreen triangle vertex shader (outputs `vert.spv`)
- `sun.frag` – Procedural solar simulation fragment shader (outputs `frag.spv`)

CMake automatically compiles these to SPIR-V if `glslc` is available, or you can compile them manually:
```bash
glslc shaders/sun.vert -o shaders/vert.spv
glslc shaders/sun.frag -o shaders/frag.spv
```

## License
MIT License – see LICENSE file.
