# Render

Render is an experimental C++23 renderer built around WebGPU through Dawn and
Slang-generated WGSL. The repository currently contains a native renderer, a
render graph, shader compilation and reflection support, asset formats, and a
small example application.

The verified development environment is Windows native x64. Linux, macOS, and
WebAssembly remain design targets rather than supported build targets.

## Implemented Areas

- WebGPU instance, device, surface, command, and resource wrappers using Dawn.
- A dependency-driven render graph and modular render-pass interface.
- Slang compilation to WGSL with reflected shader parameters and bindings.
- Shader, material, mesh, texture, pipeline, and bind-group management.
- glTF import support and an example application under `app/`.

## Repository Layout

- `app/`: Example application, sample assets, and application shaders.
- `core/`: Application framework, WebGPU renderer, render graph, passes,
  resources, and importers.
- `shader/`: Slang compiler integration, command-line compiler, and shader
  tests.
- `common/`: Shared C++/Slang interfaces and serialized asset formats.
- `cmake/` and `tools/`: Shader-asset and local build workflow support.
- `docs/`: Architecture records, ADRs, and developer guides.

See the [documentation index](docs/README.md) for maintained design documents
and their current lifecycle state.

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

No license has been selected yet.
