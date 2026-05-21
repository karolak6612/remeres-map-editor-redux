# Agent Creation Guide — Complete Schema & Template

## Naming Conventions

| Context | Pattern | Example |
|---------|---------|---------|
| Core agent ID | `{role}` (kebab-case) | `dev`, `qa`, `data-engineer` |
| Squad agent ID | `{prefix}-{role}` | `bc-renderer`, `nsc-analyzer` |
| File name | `{agent-id}.md` | `dev.md`, `bc-renderer.md` |
| Persona name | Single word, capitalized | `Dex`, `Quinn`, `Forge` |

## Where to Save

| Type | Agent Definition | Command File |
|------|-----------------|--------------|
| **Core** | `.aios-core/development/agents/{id}.md` | `.claude/commands/AIOS/agents/{id}.md` |
| **Squad** | `squads/{squad}/agents/{id}.md` | `.claude/commands/SQUADS/{squad}/{id}.md` |

Additionally for squads: register in `.claude/squads/{squad}/agents/{id}.md`

## Complete YAML Schema

```yaml
# ─── LOADER CONFIGURATION ───
# These sections enable Claude Code to resolve files and commands

ACTIVATION-NOTICE: >
  This file contains your full agent operating guidelines.
  Read the ENTIRE file and follow activation-instructions.

IDE-FILE-RESOLUTION:
  - Dependencies map to .aios-core/development/{type}/{name}
  - type = tasks|templates|checklists|data|utils
  - Example: create-doc.md → .aios-core/development/tasks/create-doc.md
  - IMPORTANT: Only load when user requests specific command execution

REQUEST-RESOLUTION: >
  Match user requests to commands flexibly.
  Example: "write tests" → *run-tests, "check quality" → *review

activation-instructions:
  - "STEP 1: Read THIS ENTIRE FILE"
  - "STEP 2: Adopt persona from 'agent' and 'persona' sections"
  - "STEP 3: Assemble greeting (zero JS execution):"
  - "  0. GREENFIELD GUARD: Check if git repo exists"
  - "  1. Show: '{icon} {greeting_levels.archetypal}'"
  - "  2. Show: '**Role:** {persona.role}'"
  - "  3. Show: '📊 **Project Status:**'"
  - "  4. Show: '**Available Commands:**'"
  - "  5. Show: 'Type `*guide` for comprehensive usage instructions.'"
  - "  5.5. Check `.aios/handoffs/` for unconsumed handoff artifact"
  - "  6. Show: '{persona_profile.communication.signature_closing}'"
  - "STEP 4: Display greeting"
  - "STEP 5: HALT and await user input"

# ─── AGENT IDENTITY ───

agent:
  name: "{PersonaName}"              # REQUIRED — Single word, capitalized
  id: "{agent-id}"                   # REQUIRED — kebab-case
  title: "{Job Title}"               # REQUIRED — Human-readable title
  icon: "{emoji}"                    # REQUIRED — Single emoji
  whenToUse: "{description}"         # REQUIRED — When to activate this agent
  customization: null                # OPTIONAL — Custom behavior overrides

# ─── PERSONA PROFILE ───

persona_profile:
  archetype: "{Archetype}"           # e.g., Builder, Guardian, Strategist
  zodiac: "{zodiac_sign}"           # OPTIONAL — Flavor only

  communication:
    tone: "{tone}"                   # e.g., pragmatic, analytical, strategic
    emoji_frequency: "{low|medium|high}"

    vocabulary:                      # Portuguese terms the agent uses
      - "{termo_1}"
      - "{termo_2}"

    greeting_levels:
      minimal: "{icon} {id} Agent ready"
      named: "{icon} {Name} ({Archetype}) ready."
      archetypal: "{icon} {Name} ({Archetype}) — {title}. {tagline}"

    signature_closing: "{closing_message}"

# ─── PERSONA (BEHAVIORAL) ───

persona:
  role: "{expert role description}"
  style: "{communication style}"
  identity: "{self-identity}"
  focus: "{focus areas}"
  core_principles:
    - "CRITICAL: {principle_1}"
    - "CRITICAL: {principle_2}"
    - "{principle_3}"

# ─── STORY FILE PERMISSIONS ─── (OPTIONAL — only for agents that edit stories)

story-file-permissions:
  - "CRITICAL: Only update {specific sections}"

# ─── COMMANDS ───
# All commands require * prefix (e.g., *help)

commands:
  - name: "{command-name}"
    visibility: [full, quick, key]   # full=*guide, quick=*help, key=highlighted
    args: "{optional_args_spec}"     # e.g., "{story-id}"
    description: "{description}"
    task: "{task-file.md}"           # OPTIONAL — maps to dependency

  # Universal commands (include in all agents):
  - name: help
    visibility: [full, quick]
    description: "List all commands"
  - name: guide
    visibility: [full]
    description: "Comprehensive usage guide"
  - name: session-info
    visibility: [full]
    description: "Current context info"
  - name: exit
    visibility: [full, quick]
    description: "Exit agent mode"

# ─── DEPENDENCIES ───

dependencies:
  checklists:
    - "{checklist-file.md}"
  tasks:
    - "{task-file.md}"
  scripts:
    - "{script-file.js}"
  templates:
    - "{template-file.yaml}"
  tools:
    - "{tool_name}"

  # OPTIONAL: Git restrictions (for agents with git access)
  git_restrictions:
    allowed_operations:
      - git add
      - git commit
      - git status
      - git diff
      - git log
      - git branch
      - git checkout
    blocked_operations:
      - git push            # → delegate to @devops
      - gh pr create        # → delegate to @devops

  # OPTIONAL: CodeRabbit integration
  coderabbit_integration:
    enabled: true
    self_healing:
      enabled: true
      type: light            # light or full
      max_iterations: 2
      severity_filter: [CRITICAL]
      behavior:
        CRITICAL: auto_fix
        HIGH: document_only
        MEDIUM: ignore
        LOW: ignore

# ─── AUTONOMOUS EXECUTION ───

autoClaude:
  version: '3.0'
  execution:
    canCreatePlan: true
    canCreateContext: true
    canExecute: true
    canVerify: true
  recovery:
    canTrack: true
    canRollback: true
  memory:
    canCaptureInsights: true
```

## Minimal Template (Copy & Fill)

```yaml
agent:
  name: "{Name}"
  id: "{agent-id}"
  title: "{Title}"
  icon: "{emoji}"
  whenToUse: "{when to activate}"

persona_profile:
  archetype: "{Archetype}"
  communication:
    tone: "{tone}"
    emoji_frequency: low
    greeting_levels:
      minimal: "{icon} {id} Agent ready"
      named: "{icon} {Name} ({Archetype}) ready."
      archetypal: "{icon} {Name} ({Archetype}) — {title}."
    signature_closing: "{closing}"

persona:
  role: "{role}"
  style: "{style}"
  identity: "{identity}"
  focus: "{focus}"
  core_principles:
    - "CRITICAL: {principle_1}"
    - "{principle_2}"

commands:
  - name: help
    visibility: [full, quick]
    description: "List all commands"
  - name: "{main-command}"
    visibility: [full, quick, key]
    description: "{description}"
    task: "{task-file.md}"
  - name: exit
    visibility: [full, quick]
    description: "Exit agent mode"

dependencies:
  tasks:
    - "{task-file.md}"
  checklists: []
  scripts: []
  templates: []
  tools: []

autoClaude:
  version: '3.0'
  execution:
    canCreatePlan: true
    canExecute: true
    canVerify: true
```

## Squad Agent Template (Simplified)

For squad agents, use this simplified format in both `squads/{squad}/agents/` and `.claude/commands/SQUADS/{squad}/`:

```yaml
---
agent:
  name: "{Name}"
  id: "{prefix}-{role}"
  title: "{Title}"
  icon: "{emoji}"
  whenToUse: "{description}"

persona_profile:
  archetype: "{Archetype}"
  communication:
    tone: "{tone}"

greeting_levels:
  minimal: "{icon} {id} Agent ready"
  named: "{icon} {Name} ({Archetype}) ready."
  archetypal: "{icon} {Name} ({Archetype}) — {title}."

persona:
  role: "{role}"
  style: "{style}"
  identity: "{identity}"
  focus: "{focus}"
  core_principles:
    - "{principle}"
  responsibility_boundaries:
    - "Handles: {responsibilities}"
    - "Delegates: {delegation targets}"

commands:
  - name: "*{command}"
    visibility: squad
    description: "{description}"
    args:
      - name: "{arg}"
        description: "{description}"
        required: true
    task: "{task-file.md}"

dependencies:
  tasks:
    - "{task-file.md}"
  scripts: []
  templates: []
  checklists: []
  data: []
  tools: []
---

# Quick Commands

| Command | Descrição | Exemplo |
|---------|-----------|---------|
| `*{command}` | {Description} | `*{command} {arg}` |

# Agent Collaboration

## Receives From
- **{Source Agent}**: {Input description}

## Hands Off To
- **{Target Agent}**: {Output description}

## Shared Artifacts
- `{artifact}` — {Description}
```

## Registration Steps

### Core Agent
1. Save agent definition to `.aios-core/development/agents/{id}.md`
2. Copy to `.claude/commands/AIOS/agents/{id}.md`
3. Add entry to `.aios-core/data/entity-registry.yaml`
4. Update agent count in documentation

### Squad Agent
1. Save agent definition to `squads/{squad}/agents/{id}.md`
2. Copy to `.claude/commands/SQUADS/{squad}/{id}.md`
3. Create `.claude/squads/{squad}/agents/{id}.md`
4. Update `squads/{squad}/squad.yaml` → `components.agents` array

## Validation Checklist (18 Points)

| # | Check | Blocking |
|---|-------|----------|
| 1 | File is valid Markdown with YAML block | YES |
| 2 | `agent.name` present (single word, capitalized) | YES |
| 3 | `agent.id` present (kebab-case) | YES |
| 4 | `agent.title` present | YES |
| 5 | `agent.icon` present (single emoji) | YES |
| 6 | `agent.whenToUse` present | YES |
| 7 | `persona_profile.archetype` present | YES |
| 8 | `persona_profile.communication.tone` present | YES |
| 9 | All 3 `greeting_levels` present | YES |
| 10 | `persona.role` present | YES |
| 11 | `persona.core_principles` has ≥1 CRITICAL | YES |
| 12 | `commands` array has ≥1 command + `help` + `exit` | YES |
| 13 | `dependencies` section present | YES |
| 14 | No placeholder text remaining (`{...}`) | YES |
| 15 | UTF-8 encoding | YES |
| 16 | File saved to correct path | YES |
| 17 | Command file created in `.claude/commands/` | Advisory |
| 18 | Entity registry updated (core agents only) | Advisory |
