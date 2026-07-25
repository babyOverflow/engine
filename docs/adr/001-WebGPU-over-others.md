---
type: adr
title: "ADR-001: Choosing WebGPU over Vulkan, DirectX, and OpenGL"
description: Rationale for selecting WebGPU-Dawn as the primary graphics API.
resource: /docs/adr/001-WebGPU-over-others.md
tags: [adr, WebGPU, rendering]
status: accepted
generated: { by: "human:babyOverflow", at: "2026-07-19T00:14:00Z" }
---

# ADR-001: Choosing WebGPU over Vulkan, DirectX, and OpenGL

## Context
The goal is to develop a modern, high-performance, cross-platform 3D game engine designed for production-ready portfolios. The engine must support modern rendering techniques while optimizing development velocity.

## Decision
We will use WebGPU (via the Dawn native implementation) as the core rendering API. The engine will interface directly with WebGPU's abstraction layer instead of introducing a custom, engine-specific rendering hardware interface (RHI).

## Rationale

* **Modern Rendering Architecture:** WebGPU provides modern graphics pipeline features—such as explicit command encoding, multi-threading capabilities, and compute shaders—matching the paradigms of Vulkan and DX12.
* **Lower Learning Curve & Reduced Boilerplate:** Unlike Vulkan, WebGPU handles low-level memory management and hazard tracking (barriers) internally. This significantly reduces boilerplate code and engineering overhead.
* **Cross-Platform Deliverability:** WebGPU offers excellent cross-platform abstractions out of the box. High deliverability is critical to proving architectural competence within a constrained development timeline.

## Consequences

* **Performance Overhead:** WebGPU introduces minor runtime overhead compared to raw Vulkan or DirectX 12 due to internal validation and abstraction layers.
* **Feature Lag:** The WebGPU specification lags behind native APIs regarding cutting-edge hardware features like bindless textures or ray-tracing extensions.

## Alternatives Considered

* **Vulkan:** Offers maximum performance but requires prohibitive development time due to explicit memory management. Risks stalling engine deliverability.
* **DirectX 12:** Industry standard for Windows/Xbox, but lacks native cross-platform capabilities, limiting target deployment.
* **OpenGL:** Legacy API. While it supports compute shaders in later versions, it suffers from severe driver overhead and lacks explicit multi-threaded command generation. It is also deprecated on Apple platforms.
* **wgpu:** A Rust-based WebGPU implementation. Excluded because C++ remains the dominant language requirement in the AAA game industry job market.
