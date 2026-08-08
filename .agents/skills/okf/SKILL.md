---
name: okf
description: Apply and validate the project's OKF v0.2 metadata, provenance, trust, lifecycle, and compatibility rules for Markdown knowledge documents.
---

# OKF v0.2 Conformance

Use this skill when a documentation task needs OKF metadata creation, review,
or migration. Read the canonical [OKF v0.2 source](../../../references/okf-v0.2.md)
before acting. Do not use the bundled `references/DEPRECATED_OKF_V0.1.md`.

## Apply

- Require a non-empty `type` for each concept document. Treat `index.md` and
  `log.md` according to the v0.2 reserved-file rules.
- Use `title`, `description`, `resource`, and `tags` when they identify the
  concept clearly.
- Record truthful `generated` metadata for new or meaningfully updated
  canonical documents.
- For an AI-generated document, identify the model in `generated.by` using
  `<provider>/<model>` (for example, `vendor/model-version`). A generic agent
  or product name without a model identifier is not sufficient. If the exact
  model is unavailable, report the attribution as unresolved instead of
  guessing.
- Preserve the original `generated` attribution for spelling, path, or other
  maintenance-only corrections that do not meaningfully regenerate the
  document.
- Record provenance in `sources`; source files are implementation evidence,
  not provenance.
- Add `verified` only after an actual human or deterministic-process check.
  Never infer, invent, or carry it forward after a meaningful content change.
- Preserve v0.1 compatibility where required: `timestamp` and body citations
  are legacy inputs, while `generated` and `sources` are the v0.2 forms.

## Boundaries

- Use `project-docs` for document structure, prose, links, code citations,
  HLA/ADR conventions, and index maintenance.
- Do not bulk-migrate documents or reformat archived material unless the task
  explicitly requests it.

## Report

State the observed OKF version, conformance issues, and any metadata whose
truth cannot be verified from the available evidence.
