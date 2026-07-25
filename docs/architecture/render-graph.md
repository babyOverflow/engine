---
type: architecture
title: render-graph architecture
description: Explains the rendering graph architecture used in the rendering engine
resource: /docs/architecture/render-graph.md
tags: [architecture, RenderGraph]
generated: { by: "human:babyOverflow", at: "2026-07-20T11:34Z" }
sources:
  - id: adr-001
    resource: ../adr/001-WebGPU-over-others.md
    title: "ADR-001: Choosing WebGPU over Vulkan, DirectX, and OpenGL"
  - id: adr-002
    resource: ../adr/002-RenderGraph-over-hardcoded-pipeline.md
    title: "ADR-002: Adopting RenderGraph over Hardcoded Pipelines"
---

# RenderGraph

## Purpose

Abstracts frame rendering into a DAG of stateless pass declarations. It compiles declared resource dependencies into an execution topology, then allocates and aliases transient GPU textures across non-overlapping passes to minimize memory overhead.

## System Components

| Type                      | Role                                                                                                 |
| ------------------------- | ---------------------------------------------------------------------------------------------------- |
| **RenderGraph**           | Compiles stateless pass declarations into a `CompiledGraph`                                          |
| **RenderQueue**           | Stores per-pass `RenderIntent` vectors, procedural pipelines and camera data submitted for execution |
| **TransientResourcePool** | Manages WebGPU resource handles and aliases for transient resources across non-overlapping passes    |
| **CompiledGraph**         | Stores all passes, their resource and topological execution order                                      |
| **SceneCuller**           | Performs frustum culling and generates per-pass render intents for the scene                         |

## Data Flow

### Architecture Overview

```mermaid
graph LR
    %% System Boundary
    subgraph SceneRenderer [Component: SceneRenderer]
        %% Component-level Inputs (Outside phase subgraphs, inside parent)
        Passes([Passes])
        RU([Render Units])


        %% Render Lifecycle Block (Right)
        subgraph RenderBlock [Render Phase]
            SC[SceneCuller]
            RQ[RenderQueue]

            %% Render Internal Flow
            SC --> RQ
        end


        %% Setup Lifecycle Block (Left)
        subgraph SetupBlock [Setup Phase]
            RG[RenderGraph]
            TRP[TransientResourcePool]
            CG[CompiledGraph]

            %% Setup Internal Flow
            RG -->|Compile| CG
            TRP <-->|request / handle| RG
        end



        %% Dependency Links (Scoped inside SceneRenderer)
        Passes --> RG
        RU --> SC
        CG -."Transfer pass target states".-> SC
        CG --> RQ
    end
```

### RenderGraph Compile Flow Chart

```mermaid
    flowchart TD
        subgraph Phase1["Phase 1: Dependency Extraction"]
            A[Pass IDs Input] --> B[Invoke IRenderPass::Setup]
            B --> C[Build VirtualPassNodes & SubResource Map]
        end

        subgraph Phase2["Phase 2: Graph Analysis & Topology"]
            C --> D[Construct Dependency Edges]
            D --> E{Cycle Check DFS}
            E -- Cycle Detected --> F[Assert Failure]
            E -- Valid DAG --> G[Generate Topological Execution Order]
        end

        subgraph Phase3["Phase 3: Resource Lifetime Scheduling"]
            G --> H[Calculate Resource Usage Intervals: firstUse / lastUse]
            H --> I[Allocate & Alias Textures via TransientResourcePool]
            I --> J[Record Surface Texture Slot: surfaceTargetLocation]
        end

        subgraph Phase4["Phase 4: Resolution & Output"]
            J --> K[Bind Pass BindGroups & Target States]
            K --> L[Emit CompiledGraph]
        end

```

## Design Decisions

We adopted a DAG-based render graph to maximize performance and scalability. [^adr-002]

The architecture is strictly constrained to WebGPU. Consequently, explicit memory aliasing and manual barrier synchronization features are omitted. [^adr-001]

## Entry Points

- The primary entry points for the [Architecture Overview](#architecture-overview) are [`RenderGraph.h`](../../core/render/graph/RenderGraph.h) and [`SceneRenderer.h`](../../core/render/SceneRenderer.h).
- The logic for the [Flow Chart](#rendergraph-compile-flow-chart) is implemented in [RenderGraph.cpp](../../core/render/graph/RenderGraph.cpp).
