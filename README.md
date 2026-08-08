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

## Build Instructions

This project uses CMake and vcpkg. The only verified build path is Windows
native x64 with the `windows-clang-debug` preset.

### Common Project Dependencies

The [vcpkg manifest](vcpkg.json) declares Dawn, GLFW, GLM, magic-enum, Slang,
tinygltf, and GoogleTest. vcpkg resolves the manifest dependencies during CMake
configuration.

### Windows Native — Verified

Install or provide the following host dependencies:

- Visual Studio C++ Build Tools and a Windows SDK.
- LLVM containing `clang-cl.exe` and `lld-link.exe`. Make its `bin` directory
  available on `PATH`, set `RENDER_LLVM_ROOT`, or pass `-LlvmRoot` to the build
  script. The script also checks the standard standalone LLVM installation and
  Visual Studio's bundled LLVM.
- CMake 3.25 or later and Ninja. The build script discovers the copies bundled
  with Visual Studio when they are not on `PATH`.
- A vcpkg checkout; set `VCPKG_ROOT` to its root directory.
- Python 3 with a `python3` command when building the full `app` target, which
  generates a C++ header from a baked shader asset.

From a PowerShell prompt:

```powershell
git clone https://github.com/babyOverflow/engine.git render
cd render

# Point this at an existing vcpkg checkout.
$env:VCPKG_ROOT = 'C:\path\to\vcpkg'

powershell -NoProfile -ExecutionPolicy Bypass -File tools\Invoke-Build.ps1 -Operation Configure
powershell -NoProfile -ExecutionPolicy Bypass -File tools\Invoke-Build.ps1 -Operation Build -Target shaderTest
powershell -NoProfile -ExecutionPolicy Bypass -File tools\Invoke-Build.ps1 -Operation Test
```

The first configure may take several minutes while vcpkg builds dependencies.
The workflow preserves vcpkg downloads, binary caches, and installed packages.

### Other Platforms

Linux, macOS, and WebAssembly are not currently supported or verified. The
CMake project and dependency manifest remain the intended foundation for
future platform workflows; platform-specific commands will be documented only
after they are implemented and validated.

## License
TBD
