# Render Repository Instructions

## Scope and Change Discipline

- Preserve unrelated work in a dirty tree. Do not use reset, checkout, or broad deletion commands.
- For explicitly planned, multi-step tasks, keep exactly one root `ACTIVE_PLAN.md` and archive it in `docs/archive/` with an ISO-date prefix when the task completes. Do not create one for routine single-step changes or review-only requests unless requested.
- Verify each changed workflow through its narrowest relevant command before reporting completion.

## Phase Checkpoints

For work organized into explicit phases, complete one phase, report its changes and validation, and await requester confirmation before starting the next. Within an approved phase, act autonomously on in-scope, reversible changes and routine validation. The requester may explicitly waive checkpoints.

## Windows Build Workflow

- Use `tools/Invoke-Build.ps1`; do not duplicate its environment setup in task files or documentation.
- Prerequisites: Visual Studio C++ Build Tools and Windows SDK, LLVM with `clang-cl` and `lld-link`, and `VCPKG_ROOT` pointing to the vcpkg checkout used by the repository. Select LLVM with `-LlvmRoot`, `RENDER_LLVM_ROOT`, or `PATH`; the script also checks standard standalone and Visual Studio LLVM locations. It discovers CMake and Ninja from Visual Studio when they are not on `PATH`.
- Commands:

  ```powershell
  powershell -NoProfile -ExecutionPolicy Bypass -File tools/Invoke-Build.ps1 -Operation Configure
  powershell -NoProfile -ExecutionPolicy Bypass -File tools/Invoke-Build.ps1 -Operation Build -Target shaderTest
  powershell -NoProfile -ExecutionPolicy Bypass -File tools/Invoke-Build.ps1 -Operation Test
  ```

- Preserve vcpkg caches and installed packages. A first configure can take several minutes while vcpkg builds dependencies.
- The application target is `app`; its expected output is `out/build/windows-clang-debug/app/app.exe`. Do not claim an application smoke test passed unless this target built and ran.

## Documentation

- Treat `docs/README.md` as the canonical documentation index. ADRs and architecture documents are maintained sources, not disposable generated output.
- Apply `project-docs` to create or review Markdown documentation under `docs/`. Use `okf` only for OKF v0.2 metadata and provenance conformance.
- Follow the project documentation style guide for document links, code references, and ADR/HLA structure. State unresolved facts as uncertainty rather than filling gaps with inferred architecture.

## Review Focus

- For WebGPU changes, check validation/error scopes, object ownership and release, callback lifetimes, descriptor initialization, pipeline/bind-group compatibility, and device-loss handling.
- For Slang changes, check entry-point/interface signatures, shader asset generation inputs, binding reflection, and WGSL output compatibility.
- Report findings with file/line evidence, severity, impact, and a minimal corrective action.
