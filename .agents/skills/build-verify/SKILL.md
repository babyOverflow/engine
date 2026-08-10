---
name: build-verify
description: Configure, build, and test the Windows LLVM/CMake workflow without clearing vcpkg caches.
---

# Build & Verify Protocol

Use this skill for Windows build or test verification. Use the repository entry point; do not reproduce Visual Studio environment setup in ad-hoc commands.

## Prerequisites

- Visual Studio C++ Build Tools and a Windows SDK.
- LLVM containing `clang-cl.exe` and `lld-link.exe`. Select it with the
  `-LlvmRoot` build-script parameter, `RENDER_LLVM_ROOT`, or `PATH`; standard
  standalone and Visual Studio LLVM locations are fallbacks.
- `VCPKG_ROOT` set to the vcpkg checkout. `RENDER_VCPKG_ROOT`, `RENDER_CMAKE_ROOT`, and `RENDER_NINJA_ROOT` optionally override discovery.

The first configure may compile vcpkg dependencies for several minutes. Never clear vcpkg caches or installed packages merely to make a verification run clean.

## Commands

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Invoke-Build.ps1 -Operation Configure
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Invoke-Build.ps1 -Operation Build -Target shaderTest
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Invoke-Build.ps1 -Operation Build -Target slangTest
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Invoke-Build.ps1 -Operation Test
```

`RunApp` searches the configured build tree for `app.exe`. Run it only after target `app` builds successfully.

## Report Results

Report the exact failing target or test plus the compiler/test diagnostic. Do not attribute existing source failures to the build environment.
