# Workflow Creation Guide — Complete Schema & Template

## Naming Conventions

| Rule | Pattern | Example |
|------|---------|---------|
| File name | `{workflow-name}.yaml` (kebab-case) | `story-development-cycle.yaml` |
| Workflow ID | `{workflow-name}` | `story-development-cycle` |
| Step IDs | `{short_descriptive}` | `create-story`, `validate`, `qa-gate` |

## Where to Save

| Type | Path |
|------|------|
| **Core** | `.aios-core/development/workflows/{name}.yaml` |
| **Squad** | `squads/{squad}/workflows/{name}.yaml` |

## Complete YAML Schema

```yaml
workflow:
  id: "{workflow-id}"                        # kebab-case, unique
  name: "{Human Readable Name}"
  version: "1.0.0"
  description: >-
    Multi-line description of workflow purpose and when to use.
  type: generic|loop|pipeline                # generic=linear, loop=iterative, pipeline=sequential
  project_types:                             # OPTIONAL — applicable project types
    - greenfield
    - brownfield
    - feature-development
    - bug-fix

  # ─── METADATA ───

  metadata:
    elicit: true                             # Requires user interaction?
    confirmation_required: true              # Needs user confirmation to proceed?
    story: "{{story_id}}"                    # Template variable
    epic: "{{epic_name}}"
    created: "{{date}}"
    author: "{{agent_name}}"
    dependencies:                            # Task files this workflow uses
      - "{task-file.md}"
    tags:
      - "{tag1}"
      - "{tag2}"

  # ─── EXECUTION MODES ───

  execution_modes:
    - mode: yolo
      description: "Execução autônoma com mínima interação"
      prompts: "0-1"
    - mode: interactive
      description: "Checkpoints de decisão e feedback"
      prompts: "5-10"
      default: true
    - mode: preflight
      description: "Planejamento completo antes da execução"
      prompts: "10-15"

  # ─── PHASES ───

  phases:
    - phase_1: "{Phase Name}"
    - phase_2: "{Phase Name}"
    - phase_3: "{Phase Name}"

  # ─── SEQUENCE ───

  sequence:
    # Normal step
    - step: "{Step Title}"
      id: "{step-id}"
      phase: 1
      agent: "{agent-id}"                   # sm, po, dev, qa, devops, etc.
      action: "{What this step does}"
      notes: |
        Detailed instructions for this step.
        Can be multi-line.
      outputs:
        - "{output_field}"
      next: "{next-step-id}"
      requires: "{dependency-step-id}"       # OPTIONAL — must complete first
      on_failure: "{fallback-step-id}"       # OPTIONAL — where to go on failure

    # Gate step (decision point)
    - step: "{Gate Title}"
      id: "{gate-id}"
      phase: 2
      agent: system
      condition_check:
        - condition: "{{steps.previous.output}} == 'GO'"
          action: continue
          log: "Gate passed"
        - condition: "{{steps.previous.output}} == 'NO-GO'"
          action: return_to
          next: "{previous-step-id}"
          log: "Gate failed, returning"

    # Conditional step
    - step: "{Conditional Step}"
      id: "{step-id}"
      phase: 3
      agent: "{agent-id}"
      condition: "{{config.mode}} == 'full'"  # Only runs if condition met
      action: "{action}"

    # Workflow end
    - workflow_end:
      id: complete
      action: workflow_complete
      notes: |
        Workflow completed successfully.
        Summary: {{steps.*.outputs}}

  # ─── FLOW DIAGRAM ───

  flow_diagram: |
    ```mermaid
    graph LR
      A[Phase 1] --> B{Gate}
      B -->|GO| C[Phase 2]
      B -->|NO-GO| A
      C --> D[Phase 3]
      D --> E[Complete]
    ```

  # ─── DECISION GUIDANCE ───

  decision_guidance:
    when_to_use:
      - "{situation 1}"
      - "{situation 2}"
    when_not_to_use:
      - "{anti-situation 1}"

  # ─── HANDOFF PROMPTS ───

  handoff_prompts:
    phase_1_to_2: |
      Fase 1 completa. Output: {{steps.step1.output}}
      Iniciando Fase 2 com {{agent_2}}...
    phase_2_to_3: |
      Fase 2 completa. Resultado: {{steps.step2.output}}

  # ─── LOOP CONFIG ─── (type: loop only)

  triggers:
    - event: command
      command: "*{trigger-command}"
      action: start_loop
    - event: verdict
      value: REJECT
      action: continue_loop

  config:
    maxIterations: 5
    statusFile: "{path/to/status.json}"
    reviewTimeout: 300000
    fixTimeout: 600000

  status_schema:
    storyId: string
    currentIteration: number
    status: "pending | in_progress | completed | stopped | escalated"
    history:
      - iteration: number
        verdict: "APPROVE | REJECT | BLOCKED"

  # ─── ESCALATION ─── (type: loop only)

  escalation:
    enabled: true
    triggers:
      - max_iterations_reached
      - verdict_blocked
      - fix_failure
      - manual_escalate

  # ─── ERROR HANDLING ───

  error_handling:
    step_failure:
      message: "Step {{step.id}} failed"
      suggestion: "Check logs and retry"
      action: prompt
    agent_unavailable:
      message: "Agent {{agent}} is unavailable"
      action: escalate
    timeout:
      message: "Step {{step.id}} timed out"
      action: prompt
```

## Template Variable Interpolation

Use `{{}}` for runtime values:

| Variable | Resolves To |
|----------|------------|
| `{{story_id}}` | Current story ID |
| `{{agent_name}}` | Current agent name |
| `{{date}}` | Current date |
| `{{steps.{id}.output}}` | Output from a specific step |
| `{{config.mode}}` | Execution mode setting |
| `{{config.maxIterations}}` | Max iterations setting |

## Minimal Template (Copy & Fill)

```yaml
workflow:
  id: "{workflow-id}"
  name: "{Workflow Name}"
  version: "1.0.0"
  description: >-
    {Description}
  type: generic

  phases:
    - phase_1: "{Phase 1 Name}"
    - phase_2: "{Phase 2 Name}"

  sequence:
    - step: "{Step 1}"
      id: step-1
      phase: 1
      agent: "{agent-id}"
      action: "{action}"
      outputs:
        - "{output}"
      next: step-2

    - step: "{Step 2}"
      id: step-2
      phase: 2
      agent: "{agent-id}"
      action: "{action}"
      requires: step-1

    - workflow_end:
      id: complete
      action: workflow_complete

  decision_guidance:
    when_to_use:
      - "{use case}"
    when_not_to_use:
      - "{anti-pattern}"

  error_handling:
    step_failure:
      message: "Step failed"
      action: prompt
```

## Step Types Reference

| Type | Structure | Use When |
|------|-----------|----------|
| **Normal** | `step`, `agent`, `action` | Agent performs an action |
| **Gate** | `condition_check` with conditions | Decision point (GO/NO-GO) |
| **Conditional** | `condition` field on step | Step only runs if condition met |
| **System** | `agent: system` | Framework-level checks |
| **End** | `workflow_end` | Terminal step |

## Registration Steps

### Core Workflow
1. Save to `.aios-core/development/workflows/{name}.yaml`
2. Add entry to `.aios-core/data/workflow-chains.yaml`
3. Add entry to `.aios-core/data/workflow-patterns.yaml`
4. Update entity-registry

### Squad Workflow
1. Save to `squads/{squad}/workflows/{name}.yaml`
2. Update `squads/{squad}/squad.yaml` → `components.workflows` array

## Validation Checklist

| # | Check | Blocking |
|---|-------|----------|
| 1 | Valid YAML syntax | YES |
| 2 | `workflow.id` present (kebab-case) | YES |
| 3 | `workflow.name` present | YES |
| 4 | `workflow.version` present (semver) | YES |
| 5 | `workflow.type` is valid (generic/loop/pipeline) | YES |
| 6 | `phases` array defined | YES |
| 7 | `sequence` has at least 2 steps + `workflow_end` | YES |
| 8 | All step `id`s are unique | YES |
| 9 | All `next`/`requires` references resolve | YES |
| 10 | All `agent` values are valid agent IDs | YES |
| 11 | Loop workflows have `config.maxIterations` | YES (if loop) |
| 12 | Error handling section present | Advisory |
| 13 | Flow diagram included | Advisory |
| 14 | File saved to correct path | YES |
