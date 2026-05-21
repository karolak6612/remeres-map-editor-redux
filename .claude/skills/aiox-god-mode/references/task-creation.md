# Task Creation Guide — Complete Schema & Template

## Naming Conventions

| Rule | Pattern | Example |
|------|---------|---------|
| File name | `{agent-id}-{verb}-{noun}.md` | `dev-develop-story.md` |
| Task function | `{agentVerb}()` (camelCase) | `devDevelopStory()` |
| Squad task | `{prefix}-{agent}-{verb}-{noun}.md` | `bc-renderer-create-html-template.md` |

Common verbs: `create`, `validate`, `review`, `execute`, `analyze`, `design`, `build`, `render`, `extract`

## Where to Save

| Type | Path |
|------|------|
| **Core** | `.aios-core/development/tasks/{name}.md` |
| **Squad** | `squads/{squad}/tasks/{name}.md` |

## Complete Task Schema

```markdown
---
task: {taskFunction}()
responsável: {AgentName} ({Archetype})
responsavel_type: Agente
atomic_layer: Template|Organism|System

# OPTIONAL — external resources
tools:
  - {tool_name}
utils:
  - {utility_name}
---

# {Task Title}

## Purpose

{Clear description of what this task accomplishes and why it exists.}

---

## Execution Modes

### 1. YOLO Mode — Fast, Autonomous (0-1 prompts)
- Execute all steps without asking
- Log decisions instead of asking

### 2. Interactive Mode — Balanced (5-10 prompts) **[DEFAULT]**
- Ask at decision points
- Educational checkpoints

### 3. Pre-Flight Planning — Comprehensive (10-15 prompts)
- Plan everything before executing
- Full approval at each phase

**Parameter:** `mode` (optional, default: `interactive`)

---

## Task Definition (AIOS Task Format V1.0)

```yaml
task: {taskFunction}()
responsável: {AgentName} ({Archetype})
responsavel_type: Agente
atomic_layer: {Layer}

**Entrada:**
- campo: {field_name}
  tipo: {string|path|object|array}
  origem: {where the value comes from}
  obrigatório: {true|false}
  validação: "{validation rule}"

**Saída:**
- campo: {field_name}
  tipo: {string|path|object|array}
  destino: {where the output goes}
  persistido: {true|false}
```

---

## Constitutional Gates

> **Reference:** Constitution Articles applicable to this task
> **Enforcement:** Automatic validation before execution

### Gate: {Gate Name} (Article {N})

```yaml
constitutional_gate:
  article: {N}
  name: "{Gate Name}"
  severity: BLOCK
  validation:
    - "{condition_1}"
    - "{condition_2}"
  on_violation:
    action: BLOCK
    message: |
      ⛔ {Error message explaining what went wrong}
```

---

## Pre-Conditions

**Purpose:** Validate prerequisites BEFORE task execution (blocking)

```yaml
pre-conditions:
  - [ ] {Condition description}
    tipo: pre-condition
    blocker: true
    validação: |
      {How to check this condition}
    error_message: "{Error if not met}"
```

---

## Steps

### Step 1: {Step Title}

{Detailed instructions for this step.}

**Expected output:** {What this step produces}

### Step 2: {Step Title}

{Instructions...}

---

## Post-Conditions

**Purpose:** Validate execution success AFTER task completes

```yaml
post-conditions:
  - [ ] {Condition description}
    tipo: post-condition
    blocker: true
    validação: |
      {How to verify}
    error_message: "{Error if not met}"
```

---

## Acceptance Criteria

```yaml
acceptance-criteria:
  - [ ] {Criterion description}
    tipo: acceptance-criterion
    blocker: true
    validação: |
      {How to verify}
```

---

## Error Handling

| Error | Action |
|-------|--------|
| {error_type} | {what to do} |

---

## Handoff

**Next agent:** {agent_id}
**Next command:** `*{command}`
**Artifacts produced:** {list of files/outputs}
```

## Minimal Template (Copy & Fill)

```markdown
---
task: {taskFunction}()
responsável: {AgentName}
responsavel_type: Agente
atomic_layer: Organism
---

# {Task Title}

## Purpose

{What this task does.}

---

## Entrada

| Campo | Tipo | Obrigatório | Origem |
|-------|------|-------------|--------|
| {field} | {type} | Sim/Não | {origin} |

## Saída

| Campo | Tipo | Destino | Persistido |
|-------|------|---------|-----------|
| {field} | {type} | {destination} | Sim/Não |

---

## Pre-Conditions

- [ ] {Condition 1}
- [ ] {Condition 2}

---

## Steps

### Step 1: {Title}

{Instructions}

### Step 2: {Title}

{Instructions}

---

## Post-Conditions

- [ ] {Verification 1}
- [ ] {Verification 2}

---

## Error Handling

| Error | Action |
|-------|--------|
| {error} | {action} |
```

## Path Resolution Rules

When an agent command references a task via `dependencies.tasks`:

```
Agent command → task file name → resolved path
  Core: .aios-core/development/tasks/{task-file}
  Squad: squads/{squad}/tasks/{task-file}
```

The `IDE-FILE-RESOLUTION` block in agent definitions handles this mapping automatically.

## Registration Steps

### Core Task
1. Save to `.aios-core/development/tasks/{name}.md`
2. Add to agent's `dependencies.tasks` array
3. Add to `.aios-core/data/entity-registry.yaml`
4. If task maps to a command, add command entry in agent definition

### Squad Task
1. Save to `squads/{squad}/tasks/{name}.md`
2. Add to agent's `dependencies.tasks` array
3. Update `squads/{squad}/squad.yaml` → `components.tasks` array

## Validation Checklist

| # | Check | Blocking |
|---|-------|----------|
| 1 | Valid Markdown with YAML frontmatter | YES |
| 2 | `task` function name present (camelCase) | YES |
| 3 | `responsável` agent specified | YES |
| 4 | Purpose section present and clear | YES |
| 5 | Entrada/Saída documented | YES |
| 6 | Pre-conditions defined | Advisory |
| 7 | Steps are numbered and actionable | YES |
| 8 | Post-conditions defined | Advisory |
| 9 | Error handling section present | Advisory |
| 10 | File saved to correct path | YES |
| 11 | Added to agent dependencies | YES |
| 12 | Constitutional gates identified (if applicable) | Advisory |
