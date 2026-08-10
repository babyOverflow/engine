---
type: index
title: Render Documentation
description: Canonical index of maintained architecture records, decisions, and developer guides.
resource: /docs/README.md
tags: [documentation, index]
status: stable
generated: { by: "openai/gpt-5.6-sol", at: "2026-08-08T07:37:17Z" }
sources:
  - id: documentation-style
    resource: guides/documentation-style.md
    title: Documentation Style Guide
---

# Render Documentation

This is the canonical index for maintained project documentation. An entry
marked **Draft** or **Placeholder** is retained intentionally but must not be
treated as a completed design.

## Architecture

- [RenderGraph](architecture/render-graph.md) — Current high-level render-graph
  data flow and entry points.
- [Render Pass](architecture/render-pass.md) — **Placeholder** for the
  render-pass architecture.

## Architecture Decisions

- [ADR-001: Choosing WebGPU](adr/001-WebGPU-over-others.md) — Accepted choice
  of WebGPU through Dawn.
- [ADR-002: Adopting RenderGraph](adr/002-RenderGraph-over-hardcoded-pipeline.md)
  — Accepted choice of a dependency-driven render graph.
- [ADR-003: Documentation Linking and Citation](adr/003-code-citation.md) —
  Accepted project-specific documentation-linking rules.

## Developer Guides

- [Documentation Style Guide](guides/documentation-style.md) — Project rules
  for metadata, links, citations, ADRs, and architecture documents.
- [Static Analysis Guide](guides/static-analysis.md) — clang-tidy and clangd
  setup for the current Windows workflow.

## Workspace Records

- [Repository instructions](../AGENTS.md) define durable agent and workflow
  rules.
- `ACTIVE_PLAN.md` appears at the repository root only while a task is active.
