---
name: code-review
description: Review C++ and Slang changes for memory safety, WebGPU correctness, shader interfaces, and rendering logic.
---

# Code Review Protocol

Use this skill for pull requests, source changes, and refactoring proposals. Base every finding on the changed code and its call path; do not infer missing architecture from stale documents.

## Required Checks

- C++: ownership and lifetime, bounds and null handling, error propagation, thread/callback synchronization, and RAII for GPU and OS resources.
- WebGPU: descriptor initialization, object/device ownership, error scopes and callbacks, device-loss paths, map/unmap sequencing, resource usage flags, and bind-group/pipeline layout compatibility.
- Slang and shader assets: interface/entry-point signatures, reflected binding names and types, generated asset inputs, WGSL compatibility, and diagnostic propagation.
- Rendering logic: frame/pass ordering, resource state assumptions, resize/recreation paths, and stale handles after cache invalidation.

## Repository Runtime Paths

- For changes affecting the window, surface, or device, trace the relevant
  flow through `Device::Create`, `CreateSurfaceForWGPU`, current-texture
  acquisition, and presentation. Verify the statuses, capabilities, and
  callbacks actually affected by the change; do not infer a platform backend
  that the source does not implement.
- For shader-pipeline changes, trace `SlangCompiler::CreateSession` through
  compilation into `ShaderManager::CreateFromShaderSource` and group-layout
  creation. Check that diagnostics, WGSL generation, and reflected bindings
  remain consistent across that path.

## Deliverable

| File & line | Severity | Evidence and impact | Minimal corrective action |
| --- | --- | --- | --- |
| `core/example.cpp:42` | Warning | Explain the concrete failure mode. | State the smallest safe change. |

Use `Critical`, `Warning`, or `Suggestion`. State explicitly when no actionable findings are present, and list remaining unverified runtime paths.
