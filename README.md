# Render Engine

An experimental, high-performance rendering engine built on **WebGPU (Dawn)** and **Slang**.

## Project Overview

This is an experimental game engine project designed with WebGPU as its first-class graphics API. It leverages the modern capabilities of WebGPU while providing a flexible, high-level abstraction through a **Render Graph** architecture.

One of the core design philosophies of this engine is the **intentional exposure of WebGPU objects** to the user-land, allowing for fine-grained control and optimization where needed, while still maintaining a structured framework for complex scene rendering.

## Key Technologies

- **Graphics API:** [WebGPU (Dawn)](https://dawn.googlesource.com/dawn) - Using Google's Dawn implementation for cross-platform, modern graphics.
- **Shader Language:** [Slang](https://shader-slang.com/) - Employing Slang for advanced shader modularity, reflection, and multi-target compilation.
- **Core Architecture:** Render Graph & Modular Render Passes.
- **Asset Pipeline:** Custom binary formats for Meshes, Materials, and Shaders with GLTF import support.

## Project Structure

- `app/`: Example application, main entry point, and high-level scene logic.
- `core/render/`: The heart of the renderer, containing:
    - `RenderGraph`: Manages resource dependencies and execution order.
    - `IRenderPass`: Modular interface for implementing various rendering stages (GBuffer, Lighting, Forward, etc.).
    - `Material/Mesh/Texture Managers`: Resource lifecycle management.
- `shader/`: Integration with the Slang compiler for runtime or build-time shader processing.
- `common/`: Shared asset format definitions and utilities.
- `wgx/`: WebGPU extensions and utility helpers (hash, convert, types).

## Current Development Status

The project is in its **early experimental stage**. Current development is heavily focused on:
1.  **Render Graph Refinement:** Improving transient resource management and pass dependency resolution.
2.  **Render Pass Implementation:** Finalizing the `IRenderPass` interface and implementing standard passes (Deferred, Lighting, etc.).
3.  **Slang Integration:** Enhancing the shader reflection and parameter binding system.

## Building the Project

This project uses **CMake** and **vcpkg** for dependency management.

### Prerequisites
- CMake 3.20+
- vcpkg
- A C++23 compatible compiler (Visual Studio 2022 recommended for Windows)

### Build Instructions
```bash
git clone <repository-url>
cd render
cmake --preset=default
cmake --build out/build/default
```

## License
TBD
