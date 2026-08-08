---
type: guide
title: Documentation Style Guide
description: Linking, citation, and formatting rules for all project documentation.
resource: /docs/guides/documentation-style.md
tags: [guide, documentation, okf]
generated: { by: "openai/gpt-5.6-sol", at: "2026-08-07T22:17:21Z" }
sources:
  - id: adr-003
    resource: ../adr/003-code-citation.md
    title: "ADR-003: Linking and Citing Content Rules"
  - id: okf-v02
    resource: ../../references/okf-v0.2.md
    title: "Open Knowledge Format (OKF) v0.2"
---

# Documentation Style Guide

This guide defines the operational rules for writing and maintaining project documentation. It implements the decisions captured in [ADR-003: Linking and Citing Content Rules](../adr/003-code-citation.md).

---

## 1. File & Cross-Document Links

Use **relative paths** for all cross-document links within the repository.

```markdown
<!-- ✅ Correct -->

[Render Graph Architecture](../architecture/render-graph.md)
[ADR-001](../adr/001-WebGPU-over-others.md)

<!-- ❌ Incorrect — absolute paths break across hosting environments -->

[Render Graph Architecture](/docs/architecture/render-graph.md)
```

> **OKF Override:** OKF v0.2 §6.1 _recommends_ absolute bundle-relative paths (`/path/from/root`) for stability when documents move within subdirectories.
> This project prefers relative links instead, as they are portable across local dev, GitHub, and static site generators.
> See [ADR-003](../adr/003-code-citation.md) for the rationale.

---

## 2. Code Citations

> [!IMPORTANT]
> **Detailed code citations are prohibited in High-Level Architecture (HLA) documents** to prevent document rot. HLA docs use a restricted `Entry Points` section instead (see §3.2). The following symbol-based formatting rules apply *only* to lower-level implementation guides, task logs, or active plans.

### 2.1 Format

Use standard Markdown links with the **symbol name in the link text** and a **relative file path** as the target.

```markdown
[<type> <SymbolName> in <FileName>](<relative-path-to-file>)
```

`<type>` (e.g., `class`, `struct`, `fn`) and the `in <FileName>` suffix are optional. Include them when they add clarity.

### 2.2 Examples

```markdown
<!-- Full form: type + symbol + file context -->

[class RenderGraph in RenderGraph.h](../../core/render/graph/RenderGraph.h)

<!-- Short form: symbol only (when file context is obvious) -->

[TransientResourcePool](../../core/render/graph/RenderGraph.h)

<!-- Method reference -->

[RenderGraph::Compile](../../core/render/graph/RenderGraph.cpp)

<!-- Cross-doc anchor (markdown-to-markdown only) -->

[Data Flow section](../architecture/render-graph.md#data-flow)
```

### 2.3 Rules

| Rule                          | Detail                                                                                                                                                            |
| ----------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **No hardcoded line numbers** | Line numbers shift on every code change. Use symbol names.                                                                                                        |
| **Symbol name = grep token**  | The link text should be directly searchable via `grep -rn "<symbol>" <file>`.                                                                                     |
| **Anchors are markdown-only** | `#anchor` fragments resolve in `.md` files (heading IDs), not in `.h`/`.cpp` source files. For source links, the symbol name in the link text is the search hint. |
| **Overloaded symbols**        | Disambiguate with return type or parameter: `[DependencyGraph Compile(span)](...)` vs `[CompiledGraph Compile(DependencyGraph&&)](...)`.                          |

### 2.4 Sources vs. Code Citations

`sources` (frontmatter) and `## Code Citations` (body) serve different purposes and must not overlap:

| Mechanism | Location | Purpose | Contents |
|---|---|---|---|
| `sources` | YAML frontmatter | **Provenance** — what external documents informed this doc | ADRs, specs, design references, external URLs |
| `## Code Citations` / `## Entry Points` | Markdown body | **Traceability** — where in the codebase a concept manifests | Links to `.h`/`.cpp` source files |

**Boundary rule:** Source code files are never listed in `sources` (they are not provenance documents). Specs, ADRs, and external references are never listed in `## Code Citations` or `## Entry Points` (they are not code).

---

## 3. Document Structure

### 3.1 Frontmatter

All documents under `docs/` must begin with OKF-compliant YAML frontmatter:

```yaml
---
type: architecture | adr | guide | spec
title: "<Document Title>"
description: "<One-line summary>"
resource: /docs/<path>/<filename>.md
tags: [<relevant>, <tags>]
generated: { by: "<actor>", at: "<ISO 8601 datetime>" } # actor: "human:<id>", "<provider>/<model>", or "process:<id>"
sources:
  - id: <source-key>
    resource: <relative-path-or-url>
    title: "<Human-readable label>"
---
```

> **OKF v0.2 Migration:** The v0.1 `timestamp` field is superseded by `generated: { by, at }` (OKF v0.2 §5.2, §13.1).
> The v0.1 `date` field used in our ADRs serves the same purpose and should also migrate to `generated.at`.
> Use `generated` in all new documents. Existing docs with `timestamp` or `date` should be migrated.

For AI-generated content, `generated.by` must include the model identifier,
such as `openai/gpt-5.6-sol` or `antigravity/gemini-3.1-pro`. A generic product
or agent name is insufficient evidence for trust decisions. Do not invent a
model identifier when it is unavailable. Spelling, path, and other
maintenance-only corrections do not replace the original document's
`generated` attribution.

### 3.2 High-Level Architecture (HLA) Documents

HLA documents focus on macro-level data flow and system design. **Exhaustive implementation details (e.g., mapping every class or symbol) are strictly prohibited** to prevent document rot. Traceability is restricted to a minimal `Entry Points` section.

Follow this section order:

1. **Purpose** — 2–3 sentences. What macro-level problem does this subsystem solve?
2. **System Components** — Table: Component | Macro-level Role (No specific code symbols)
3. **Data Flow** — Mermaid diagram preferred (Block-diagram level)
4. **Design Decisions** — Link to relevant ADRs
5. **Entry Points** — Link to the 1–2 primary source files where the subsystem begins (e.g., `core/render/graph/RenderGraph.h`). Do not list exhaustive symbol details.

Architecture docs should also list their informing ADRs and specs in `sources` frontmatter (§2.4).

### 3.3 Architecture Decision Records (ADRs)

Follow this section order:

1. **Context** — Problem or constraint
2. **Decision** — What was chosen
3. **Rationale** — Why (separate from Decision for scannability)
4. **Consequences** — Trade-offs accepted
5. **Alternatives Considered** — What was rejected and why

ADRs document _decisions_, not _implementations_. **Code citations are not required** in ADRs. If an ADR has a corresponding architecture doc, add an `## Implementation Reference` link at the bottom.

---

## 4. Language

- **Primary language:** English.
- **Korean summaries:** Permitted for complex architecture explanations, placed in a clearly labeled subsection (e.g., `### 아키텍처 요약`).
- **In-code comments:** Korean is acceptable; documentation files should be English.

---

## 5. Hygiene

| Location             | Rule                                                                    |
| -------------------- | ----------------------------------------------------------------------- |
| `docs/` root         | Only `README.md` and `overview.md`.                                     |
| `docs/architecture/` | One file per subsystem. No WIP drafts.                                  |
| `docs/adr/`          | Numbered ADRs only. Format: `NNN-<slug>.md`.                            |
| `docs/guides/`       | Task-oriented how-to documents.                                         |
| `docs/archive/`      | Date-prefixed completed plans only.                                     |
| Repo root            | Exactly one `ACTIVE_PLAN.md` for in-flight work. Archive on completion. |
