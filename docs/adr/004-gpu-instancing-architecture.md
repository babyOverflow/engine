---
type: adr
title: "ADR-004: GPU Instancing Architecture & Buffer Binding Strategy"
description: Rationale for instance data binding and per-frame buffer management.
resource: /docs/adr/004-gpu-instancing-architecture.md
tags: [adr, rendering, instancing]
status: proposed
generated: { by: "antigravity/gemini-3.1-pro", at: "2026-07-27" }
---

# ADR-004: GPU Instancing Architecture & Buffer Binding Strategy

## Context
To support efficient rendering of identical objects (e.g., forests, particles), the engine must implement GPU instancing. The core challenges involve deciding how instance data (e.g., local-to-world transforms) is uploaded to the GPU per-frame, and how it is bound and accessed by shaders without introducing significant CPU overhead or pipeline complexity.

## Alternatives Considered

### Option 1: Instanced Vertex Buffers (`VertexStepMode::Instance`)
Instance data is uploaded to a Vertex Buffer. The pipeline is configured to read from this buffer per-instance.
*   **Pros:** Native hardware path; highly compatible; avoids storage buffer limits.
*   **Cons:** Matrix data (4x `float4`) consumes 4 vertex attribute slots (out of a max of 16). Tightly couples instance data to the vertex layout, complicating pipeline cache management.

### Option 2: Dynamic Uniform Buffer (`SetBindGroup` with dynamic offsets)
A large Uniform Buffer is uploaded per-frame. Each draw call updates the dynamic offset to point to the correct instance data.
*   **Pros:** Fast uniform access.
*   **Cons:** Requires calling `SetBindGroup` per draw call, which incurs high CPU overhead and defeats the primary goal of instancing (reducing API calls). Data must be aligned to `minUniformBufferOffsetAlignment` (often 256 bytes), wasting memory for 64-byte matrices.

### Option 3: Global Storage Buffer (`StructuredBuffer` via Bind Group)
A single, global `wgpu::Buffer` (`wgpu::BufferUsage::Storage`) is created. All transforms for a frame are batched into a contiguous block and uploaded via `wgpu::Queue::WriteBuffer` once per frame. Shaders read from a `StructuredBuffer` using `baseInstance + SV_InstanceID`.
*   **Pros:** 
    *   **Zero per-draw binding overhead:** The buffer is bound once globally or per-pass.
    *   **Maximal batching:** A single `DrawIndexed` call can draw thousands of instances.
    *   **Extensible:** Easily expanded to hold per-instance material overrides (e.g., colors).
*   **Cons:** Storage buffers can have slightly higher latency on older mobile hardware compared to Uniform buffers (negligible on modern desktop GPUs).

## Proposed Decision
We will adopt **Option 3: Global Storage Buffer (`StructuredBuffer`)** with specialized, tightly packed structures.

### Architecture Details:
1.  **Data Structures & Alignment:** 
    *   Instance data is split into `StaticInstanceData` and `SkinnedInstanceData`.
    *   Both are `alignas(16)` 64-byte structures using three `vec4` rows for the matrix, reserving the remaining 16 bytes for `customDataIndex`, `boneBufferOffset`, and padding.
2.  **Buffer Management & Overhead (`InstanceQueue`):** 
    *   `SceneRenderer` will maintain an `InstanceQueue` containing two persistent `wgpu::Buffer` objects (one for each data type), sized to accommodate the maximum expected instances.
    *   **Per-frame Overhead:** A single `std::vector` copy on the CPU to flatten the batched instance data, followed by a non-blocking `wgpu::Queue::WriteBuffer` call for each buffer.
3.  **Bind Group Strategy:**
    *   The storage buffers will be bound to **Group 0: Global Scene Data**, aligning their lifecycle with camera and scene-wide uniforms.
4.  **Shader Integration:**
    *   Slang shaders will declare `StructuredBuffer<StaticInstanceData>` and `StructuredBuffer<SkinnedInstanceData>` in the global scope.
    *   The vertex shader will fetch matrices via the appropriate buffer using `baseInstance + SV_InstanceID`.

## Consequences
- Requires modifying existing shaders to accept a `baseInstance` offset (likely via push constants or a secondary tiny uniform, or by manually offsetting `SV_InstanceID` if WebGPU's `firstInstance` parameter is exposed and supported).
- Drastically reduces CPU-side WebGPU API calls, shifting the bottleneck to GPU geometry throughput.
