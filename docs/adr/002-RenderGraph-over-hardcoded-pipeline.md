---
type: adr
title: "ADR-002: Adopting RenderGraph over Hardcoded Pipelines"
description: Rationale for implementing a Directed Acyclic Graph (DAG) based rendering system.
resource: /docs/adr/002-RenderGraph-over-hardcoded-pipeline.md
tags: [adr, rendering, RenderGraph]
status: stable
generated: { by: "human:babyOverflow", at: "2026-07-19T05:24:00Z" }
---

# ADR-002: Adopting RenderGraph over Hardcoded Pipelines

## Context
As the rendering engine expands to support complex multi-pass techniques, hardcoded pipelines fail to scale. Managing render passes manually creates tight coupling, increases resource allocation bugs, and lacks automated pass ordering.

## Decision
We will implement a Directed Acyclic Graph (DAG) based execution management system designated as `RenderGraph`. This system automates resource lifecycle management and pass scheduling while providing a flexible, scalable interface for adding new rendering passes.

## Rationale
- **Industry Alignment:** DAG-based rendering architectures are the industry standard across modern game engines. Adopting this proven pattern reduces architectural risk and leverages well-documented implementation paradigms.
- **Decoupled Pass Configuration:** Separates pass declaration from command execution. Developers can add or remove rendering passes without modifying core engine rendering loops.
- **Automated Pass Scheduling:** Resolves pass execution order dynamically based on declared resource dependencies rather than relying on hardcoded, fragile execution sequences.
- **Transient Resource Efficiency:** Optimizes memory allocations by analyzing resource lifecycles across passes, allowing non-overlapping transient resources to share allocations and reducing runtime overhead.

## Consequences
- **Upfront Architectural Complexity:** Designing a robust graph compiler requires significant engineering effort compared to a linear, hardcoded execution path.
- **CPU Compilation Overhead:** Rebuilding or optimizing the graph topology introduces minor CPU overhead at runtime.

## Alternatives Considered
- **Hardcoded Linear Pipeline:** Simple to implement initially, but introduces severe technical debt. It forces static resource allocation, prevents transient resource reuse, and makes cross-pass optimization impossible as the renderer scales.

## Implementation Reference

See [RenderGraph Architecture](../architecture/render-graph.md) for implementation details.
