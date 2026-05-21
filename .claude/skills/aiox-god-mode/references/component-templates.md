# Component Templates Guide — Checklists, Templates, Data Files & Rules

## 1. Checklist Creation

### Where to Save

| Type | Path |
|------|------|
| **Core** | `.aios-core/development/checklists/{name}.md` |
| **Squad** | `squads/{squad}/checklists/{name}.md` |

### Naming: `{domain}-{purpose}-checklist.md` (e.g., `pre-push-checklist.md`, `layout-correctness-checklist.md`)

### Schema

```markdown
# {Checklist Title}

```yaml
checklist:
  id: "{checklist-id}"
  version: "1.0.0"
  created: "{date}"
  updated: "{date}"
  purpose: "{what this checklist validates}"
  mode: blocking|advisory           # blocking = prevents progress, advisory = warns only
  reference: "{path to related docs}"
```

---

## Category 1: {Category Name}

```yaml
checks:
  - id: "{cat}-{num}"
    check: "{what to verify}"
    type: blocking|recommended
    validation: "{how to check}"
```

- [ ] **{id}**: {check description}
  - Type: {blocking|recommended}
  - Validation: `{command or manual check}`

- [ ] **{id}**: {check description}
  - Type: {blocking|recommended}

---

## Category 2: {Category Name}

{Same structure...}

---

## Scoring

| Result | Criteria |
|--------|----------|
| **PASS** | All blocking checks pass |
| **CONCERNS** | Blocking pass, some recommended fail |
| **FAIL** | Any blocking check fails |
```

### Minimal Template

```markdown
# {Name} Checklist

```yaml
checklist:
  id: "{id}"
  version: "1.0.0"
  purpose: "{purpose}"
  mode: blocking
```

## Checks

- [ ] **C-01**: {Check 1} — Type: blocking
- [ ] **C-02**: {Check 2} — Type: blocking
- [ ] **C-03**: {Check 3} — Type: recommended

## Scoring

| Result | Criteria |
|--------|----------|
| PASS | All blocking checks pass |
| FAIL | Any blocking check fails |
```

---

## 2. Template Creation

### Where to Save

| Type | Path |
|------|------|
| **Core** | `.aios-core/development/templates/{name}.{md\|yaml}` |
| **Squad** | `squads/{squad}/templates/{name}.{md\|yaml}` |

### Variable Syntax

| Syntax | Meaning |
|--------|---------|
| `{{VARIABLE}}` | Required placeholder — MUST be replaced |
| `{{variable\|default}}` | Optional with default value |
| `{{#if condition}}...{{/if}}` | Conditional section |
| `{{#each items}}...{{/each}}` | Loop over array |

### Markdown Template Schema

```markdown
# {{TITLE}}

**Version:** {{VERSION}}
**Last Updated:** {{DATE}}
**Status:** {{STATUS}}

---

## Overview

{{OVERVIEW_TEXT}}

---

## {{SECTION_1_TITLE}}

### {{SUBSECTION_TITLE}}

{{CONTENT}}

```{{LANGUAGE}}
// Code example
{{CODE_EXAMPLE}}
```

---

## Related Documents

- [{{DOC_NAME}}]({{DOC_PATH}})
```

### YAML Template Schema

```yaml
# {{TEMPLATE_NAME}} Template
# Version: {{VERSION}}

{{root_key}}:
  name: "{{NAME}}"
  version: "{{VERSION}}"
  description: "{{DESCRIPTION}}"

  {{#if has_config}}
  config:
    {{CONFIG_KEY}}: "{{CONFIG_VALUE}}"
  {{/if}}

  {{#each items}}
  - name: "{{this.name}}"
    value: "{{this.value}}"
  {{/each}}
```

---

## 3. Data File Creation

### Where to Save

| Type | Path |
|------|------|
| **Core** | `.aios-core/data/{name}.yaml` |
| **Squad** | `squads/{squad}/data/{name}.{yaml\|json}` |

### Registry Format

Registries track entities (tools, agents, patterns) with metadata:

```yaml
# {Registry Name}
# Version: 1.0.0
# Updated: {date}
# Purpose: {what this registry tracks}

registry:
  version: "1.0.0"
  updated: "{date}"
  description: "{purpose}"

entries:
  - id: "{entry-id}"
    name: "{Entry Name}"
    type: "{type}"
    path: "{file path}"
    layer: "L1|L2|L3|L4"
    purpose: "{what it does}"
    keywords:
      - "{keyword1}"
      - "{keyword2}"
    usedBy:
      - "{agent-id}"
    dependencies:
      - "{dep-id}"
```

### Heuristics Format

Heuristics define decision rules and patterns:

```yaml
# {Heuristics Name}
# Purpose: {what decisions these help with}

heuristics:
  version: "1.0.0"

  rules:
    - id: "{rule-id}"
      trigger: "{when to apply}"
      condition: "{what to check}"
      action: "{what to do}"
      confidence: 0.9             # 0-1 confidence score
      source: "{where this rule came from}"

  patterns:
    - id: "{pattern-id}"
      name: "{Pattern Name}"
      detection: "{how to detect this pattern}"
      response: "{what to do when detected}"
      examples:
        - "{example input}"
```

### Workflow Patterns/Chains Format

```yaml
# Workflow Patterns
version: "1.0.0"

patterns:
  - id: "{pattern-id}"
    name: "{Pattern Name}"
    detection:
      keywords: ["{keyword1}", "{keyword2}"]
      context: "{when this applies}"
    workflow: "{workflow-id}"
    confidence: 0.85
    next_steps:
      - priority: 1
        action: "{action}"
        agent: "{agent-id}"
```

---

## 4. Rule Creation

### Where to Save

Rules go in `.claude/rules/{rule-name}.md`. They are auto-loaded by Claude Code based on frontmatter `paths:`.

### Rule Schema

```markdown
---
paths:
  - "{glob pattern 1}"            # e.g., "docs/stories/**"
  - "{glob pattern 2}"            # e.g., ".aios-core/development/**"
---

# {Rule Name} — {Short Description}

## Purpose

{Why this rule exists and what it enforces.}

## Rules

### Rule 1: {Title}

{Description of the rule.}

**Enforcement:** {BLOCK|WARN|INFO}

### Rule 2: {Title}

{Description...}

## Examples

### Correct
```
{correct example}
```

### Incorrect
```
{incorrect example}
```

## Exceptions

- {When this rule doesn't apply}
```

### Conditional Loading

The `paths:` frontmatter controls WHEN the rule loads:

| Pattern | Loads When |
|---------|-----------|
| `docs/stories/**` | Editing any story file |
| `.aios-core/**` | Working with framework files |
| `packages/**` | Working with project packages |
| `tests/**` | Working with test files |
| No `paths:` | Always loaded (global rule) |

### Minimal Rule Template

```markdown
---
paths:
  - "{when to load}"
---

# {Rule Name}

## {Rule Description}

{What to do and what not to do.}

| Do | Don't |
|----|-------|
| {correct} | {incorrect} |
```

---

## Validation Summary

| Component | Key Checks |
|-----------|-----------|
| **Checklist** | Valid YAML metadata, blocking/advisory types, scoring defined |
| **Template** | All `{{VARIABLES}}` documented, sections structured, UTF-8 |
| **Data File** | Valid YAML, version present, entries schema consistent |
| **Rule** | `paths:` frontmatter (if conditional), enforcement levels defined |
