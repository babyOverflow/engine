---
type: adr
title: "ADR-003: Linking and Citing Content Rules"
description: "Establishes relative linking and symbol-based citation standards for documentation."
resource: /docs/adr/003-code-citation.md
tags: [adr, documentation, okf]
status: accepted
generated: { by: "human:babyOverflow", at: "2026-07-23T09:34:28Z" }
sources:
  - id: okf-v02
    resource: ../guides/SPEC.md
    title: "Open Knowledge Format (OKF) v0.2"
---

# ADR-003: Linking and Citing Content Rules

## Context

We have adopted the Open Knowledge Format (OKF) v0.2 as our documentation standard for both human engineers and AI agents. Because OKF is an evolving specification, discrepancies exist between spec recommendations and real-world tool behavior (e.g., GitHub, VS Code, LSPs). We need clear linking and citation conventions to ensure cross-platform portability and resilience against code refactoring.

## Decisions

### 1. File & Cross-Document Links

Define rules for referencing external documents or assets within the repository.

* **Decision**: Strongly recommend **relative links** over absolute links.
* **Rationale**: Markdown tools (e.g., GitHub, VS Code, static site generators) resolve absolute paths starting with `/` relative to the system root or repository root. Moving documents or hosting them in subdirectories (e.g., GitHub Pages) breaks absolute links.
* **Consequences**:
  * Documents remain portable across local environments, web platforms, and subdirectories.
  * Moving a file requires updating relative paths within that file.

---

### 2. Code Fragment & Symbol Citations

Define rules for referencing specific functions, classes, and code segments inside files.

* **Decision**: Avoid hardcoded line numbers. Use relative Markdown links with symbol names and explicit anchors instead.
* **Recommended Formats**:
  * **Inline Anchor Link:** `[<title>](#<anchor-name>)`
  * **External File / Symbol Link:** `[<type> <title> in <file-name>](<file-path>#<anchor-name>)`

> [!note]
> Note: `<type>` and `#<anchor-name>` are optional. Omit them if not applicable.

#### Examples
```markdown
[File & Cross-Document Links section](#1-file--cross-document-links)
[RenderGraph Architecture](../architecture/render-graph.md#data-flow)
[struct TransientResourcePool in RenderGraph.h](../../core/render/graph/RenderGraph.h)
```

* **Rationale**:
  * Line-number citations are brittle; any code change or reformatting invalidates them.
  * Standard Markdown parsers natively support section anchors (`#anchor-name`).
  * Explicit symbol names (`struct`, `class`, `fn`) provide high-signal context for AI agents and LSP tools.
* **Consequences**:
  * Documentation references do not break when upstream source code shifts.
  * Authors must write descriptive anchor text instead of pasting raw line numbers.

## Implementation Reference

See [Documentation Style Guide](../guides/documentation-style.md) for full operational linking, citation, and document structure guidelines.
