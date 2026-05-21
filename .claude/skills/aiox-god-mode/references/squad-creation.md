# Squad Creation Guide — Complete Structure & Template

## What is a Squad

A squad is a self-contained collection of agents, tasks, workflows, and config that work together toward a specific domain. Squads live in `squads/{name}/` and are registered in `.claude/` for activation.

## Naming Conventions

| Element | Pattern | Example |
|---------|---------|---------|
| Squad name | `{kebab-case}` | `brandcraft`, `nirvana-squad-creator` |
| Slash prefix | `{short}` (2-4 chars) | `bc`, `nsc`, `lp` |
| Agent ID | `{prefix}-{role}` | `bc-renderer`, `nsc-analyzer` |
| Task ID | `{prefix}-{agent-role}-{verb}-{noun}.md` | `bc-renderer-create-html-template.md` |

## Directory Structure

```
squads/{squad-name}/
├── squad.yaml                    # Manifest (REQUIRED)
├── README.md                     # Documentation
├── README.{lang}.md              # i18n: pt, es, en, hi, zh, ar
├── config/
│   ├── coding-standards.md       # Squad-specific coding standards
│   ├── tech-stack.md             # Technologies used
│   └── source-tree.md            # File structure reference
├── agents/                       # Agent definitions
│   ├── {prefix}-{role}.md
│   └── ...
├── tasks/                        # Task definitions
│   ├── {prefix}-{role}-{verb}-{noun}.md
│   └── ...
├── workflows/                    # Multi-step workflows
│   ├── {workflow-name}.yaml
│   └── ...
├── checklists/                   # Validation checklists
│   ├── {checklist-name}.md
│   └── ...
├── templates/                    # Document/code templates
│   ├── {template-name}.md
│   └── ...
├── tools/                        # Custom tools/scripts
│   └── {tool}.js
├── scripts/                      # Utility scripts
│   └── {script}.js
├── data/                         # Static data files
│   └── {data}.json
└── references/                   # Reference documentation
    └── {ref}.md
```

## Squad Manifest Schema (squad.yaml)

```yaml
name: "{squad-name}"                       # kebab-case (REQUIRED)
version: "1.0.0"                           # Semantic versioning (REQUIRED)
description: "{description}"               # Purpose of the squad (REQUIRED)
author: "{author}"                         # Creator name (REQUIRED)
license: MIT                               # MIT, Apache-2.0, ISC, UNLICENSED

slashPrefix: "{prefix}"                    # Short activation prefix (REQUIRED)

aios:
  minVersion: "2.1.0"                      # Minimum AIOS version required
  type: squad

# ─── COMPONENTS ───

components:
  agents:
    - "{prefix}-{role}.md"
    - "{prefix}-{role2}.md"
  tasks:
    - "{prefix}-{role}-{verb}-{noun}.md"
  workflows:
    - "{workflow-name}.yaml"
  checklists:
    - "{checklist-name}.md"
  templates:
    - "{template-name}.md"
  tools: []
  scripts:
    - "{script}.js"

# ─── CONFIG ───

config:
  extends: none                            # none, extend, override
  coding-standards: config/coding-standards.md
  tech-stack: config/tech-stack.md
  source-tree: config/source-tree.md

# ─── DEPENDENCIES ───

dependencies:
  node:
    - "puppeteer@^23.0.0"
  python: []
  squads: []                               # Other squads this depends on

# ─── MCP TOOLS (OPTIONAL) ───

mcpTools:
  required:
    - "{tool}": "{why required}"
  optional:
    - "{tool}": "{nice to have}"

# ─── CUSTOM FORMATS (OPTIONAL) ───

documentFormats:
  - name: "{format-name}"
    dimensions: "{WxH or standard}"
    technology: "{HTML|PPTX|etc}"

videoFormats:
  - name: "{format-name}"
    dimensions: "{WxH}"
    fps: 30
    technology: "{Remotion|FFmpeg|etc}"

# ─── METADATA ───

tags:
  - "{domain}"
  - "{capability}"
```

## Registration Steps

After creating the squad directory and files:

### 1. Create Squad Registration

```
.claude/squads/{squad-name}/
└── agents/
    ├── {prefix}-{role1}.md      # Copy of each agent definition
    ├── {prefix}-{role2}.md
    └── ...
```

### 2. Create Command Files

```
.claude/commands/SQUADS/{squad-name}/
├── {prefix}-{role1}.md          # Agent command definition
├── {prefix}-{role2}.md
└── ...
```

These command files enable `/SQUADS:{squad-name}:{agent-id}` activation.

### 3. Update Skill Description (if needed)

If the squad should appear in `/aios-god-mode`'s squad list, update the Squads table in SKILL.md.

## Agent Collaboration Patterns

### Pipeline Pattern
Agents work sequentially, each receiving output from the previous:
```
Agent A → Agent B → Agent C → Agent D
```
Example: BrandCraft `extractor → templater → renderer → presenter`

### Hub-and-Spoke Pattern
One orchestrator agent coordinates all others:
```
        ┌─ Agent B
Agent A ─┤─ Agent C
        └─ Agent D
```
Example: NSC `orchestrator → analyzer, agent-creator, task-creator, ...`

### Review Pattern
Work agent + reviewer with feedback loop:
```
Agent A ──→ Agent B ──→ [PASS] → Done
              │
              └─ [FAIL] → Agent A (fix) → Agent B (re-review)
```

### Define Collaboration in Agent Files

Each squad agent should document:
- **Receives From**: Which agent(s) provide input
- **Hands Off To**: Which agent(s) receive output
- **Shared Artifacts**: Common files/data between agents

## Minimal Squad Template

### squad.yaml
```yaml
name: "{squad-name}"
version: "1.0.0"
description: "{description}"
author: "{author}"
license: MIT
slashPrefix: "{prefix}"

aios:
  minVersion: "2.1.0"
  type: squad

components:
  agents:
    - "{prefix}-{role1}.md"
    - "{prefix}-{role2}.md"
  tasks:
    - "{prefix}-{role1}-{verb}-{noun}.md"
  workflows: []
  checklists: []
  templates: []
  tools: []
  scripts: []

config:
  extends: none
  coding-standards: config/coding-standards.md
  tech-stack: config/tech-stack.md

dependencies:
  node: []
  python: []
  squads: []

tags:
  - "{tag1}"
  - "{tag2}"
```

### config/coding-standards.md
```markdown
# {Squad Name} — Coding Standards

## Language & Style
- {coding rules specific to this squad}

## File Organization
- {how files should be organized}

## Naming Conventions
- {naming rules}
```

### config/tech-stack.md
```markdown
# {Squad Name} — Tech Stack

## Core Technologies
| Technology | Purpose | Version |
|-----------|---------|---------|
| {tech} | {purpose} | {version} |
```

## Creation Workflow (Step by Step)

1. **Elicit requirements**: name, purpose, agents needed, collaboration pattern
2. **Create directory**: `mkdir -p squads/{name}/{agents,tasks,workflows,config,checklists}`
3. **Write squad.yaml**: manifest with all components
4. **Create agent files**: one per agent in `agents/`
5. **Create task files**: one per command in `tasks/`
6. **Create config files**: coding-standards, tech-stack
7. **Write README.md**: documentation
8. **Register squad**: create files in `.claude/squads/` and `.claude/commands/SQUADS/`
9. **Validate**: run squad validation checklist

## Validation Checklist

| # | Check | Blocking |
|---|-------|----------|
| 1 | `squad.yaml` exists and is valid YAML | YES |
| 2 | `name` is kebab-case | YES |
| 3 | `version` follows semver | YES |
| 4 | `slashPrefix` is unique (not used by other squads) | YES |
| 5 | All files in `components.agents` exist | YES |
| 6 | All files in `components.tasks` exist | YES |
| 7 | All agent IDs start with `{prefix}-` | YES |
| 8 | All task IDs follow naming convention | Advisory |
| 9 | `config/coding-standards.md` exists | Advisory |
| 10 | `config/tech-stack.md` exists | Advisory |
| 11 | Agent collaboration documented (receives/hands-off) | Advisory |
| 12 | README.md exists | Advisory |
| 13 | Registration in `.claude/squads/` complete | YES |
| 14 | Registration in `.claude/commands/SQUADS/` complete | YES |
| 15 | No naming conflicts with existing squads | YES |
