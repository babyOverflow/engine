---
name: project-docs
description: Create, revise, or review Markdown documentation under docs using the project's documentation style guide, ADR conventions, and code-evidence workflow.
---

# Project Documentation Workflow

Use this skill for canonical Markdown documentation under `docs/`. Read the
[Documentation Style Guide](../../../docs/guides/documentation-style.md)
before acting.

## Classify and Gather Evidence

- Classify the requested artifact as an ADR, high-level architecture document,
  implementation guide, task record, or index.
- Read the relevant existing ADRs, architecture documents, and source files
  before making technical claims. Do not reconstruct missing design from names
  or stale documents.
- Use the `okf` skill when the task creates, changes, or audits OKF metadata.

## Write or Revise

- Follow the style guide's structure for the selected document type.
- Use relative links for repository documents. Use source-file links with
  searchable symbol names for implementation references.
- Keep high-level architecture documents at macro level; use only their
  minimal `Entry Points` section for code traceability.
- Keep ADRs focused on decisions and consequences, not implementation detail.
- Update an index only when the task changes its canonical contents. Do not
  fabricate a missing document to satisfy a link.

## Review

- Check document type, required sections, links, sources, and implementation
  evidence against the style guide.
- Report confirmed facts, assumptions, stale references, and decisions that
  require human confirmation. Do not modify files during a review-only task.
