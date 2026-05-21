# Framework Map — Complete Navigation Reference

## 4-Layer Boundary System

| Layer | Mutability | Paths | Enforced By |
|-------|-----------|-------|-------------|
| **L1** Framework Core | NEVER modify | `.aios-core/core/`, `.aios-core/constitution.md`, `bin/aios.js`, `bin/aios-init.js` | deny rules in `.claude/settings.json` |
| **L2** Framework Templates | NEVER modify (extend-only) | `.aios-core/development/tasks/`, `.aios-core/development/templates/`, `.aios-core/development/checklists/`, `.aios-core/development/workflows/`, `.aios-core/infrastructure/` | deny rules |
| **L3** Project Config | Mutable (with care) | `.aios-core/data/`, `agents/*/MEMORY.md`, `core-config.yaml` | allow rules |
| **L4** Project Runtime | ALWAYS modify | `docs/stories/`, `packages/`, `squads/`, `tests/` | working area |

**Toggle:** `core-config.yaml` → `boundary.frameworkProtection: true/false`

---

## Component Locator

| Component | Location | Count |
|-----------|----------|-------|
| Agent definitions | `.aios-core/development/agents/{agent-id}/` | 11 |
| Agent memory | `.aios-core/development/agents/{agent-id}/MEMORY.md` | 11 |
| Executable tasks | `.aios-core/development/tasks/` | 207+ |
| Workflow definitions | `.aios-core/development/workflows/` | 15 |
| Templates | `.aios-core/development/templates/` | 8+ |
| Checklists | `.aios-core/development/checklists/` | 9 |
| Constitution | `.aios-core/constitution.md` | 1 |
| Rules | `.claude/rules/` | 8 |
| Handoff artifacts | `.aios/handoffs/` | runtime |
| Squads | `.claude/squads/` | 5 |
| Commands | `.claude/commands/` | 60+ |
| Skills | `.claude/skills/` | varies |
| Core config | `core-config.yaml` | 1 |
| Workflow chains | `.aios-core/data/workflow-chains.yaml` | 1 |
| Knowledge base | `.aios-core/data/aios-kb.md` | 1 |
| Stories | `docs/stories/` | varies |
| PRDs | `docs/prd/` | varies |
| Architecture docs | `docs/architecture/` | varies |

---

## Task Resolution Pattern

```
User request
  → Match to agent command
  → Resolve to task file via dependencies
  → Path: .aios-core/development/{type}/{name}

Example:
  "draft story" → @sm *draft → create-next-story.md
  → .aios-core/development/tasks/create-next-story.md
```

---

## Rules System

| Rule File | Description | Auto-loads When |
|-----------|-------------|----------------|
| `agent-authority.md` | Delegation matrix, exclusive ops | Agent context |
| `agent-handoff.md` | Context compaction protocol | Agent switching |
| `agent-memory-imports.md` | Agent memory lifecycle | Memory operations |
| `coderabbit-integration.md` | Code review integration | Code review |
| `ids-principles.md` | Incremental Development gates | Component creation |
| `mcp-usage.md` | MCP tool selection priority | Tool usage |
| `story-lifecycle.md` | Story transitions, quality gates | Story files |
| `workflow-execution.md` | 4 primary workflows | Workflow execution |
| `tool-examples.md` | Concrete tool input examples | Tool usage |
| `tool-response-filtering.md` | Output filtering rules | Tool responses |

---

## Checklists

| Checklist | Used By | When |
|-----------|---------|------|
| `story-draft-checklist.md` | @sm | Before @po review |
| `story-dod-checklist.md` | @dev/@qa | Definition of Done |
| `po-master-checklist.md` | @po | Story validation |
| `pre-push-checklist.md` | @devops | Before push |
| `self-critique-checklist.md` | @dev | Self-review |
| `agent-quality-gate.md` | All | Activation quality |
| `memory-audit-checklist.md` | @aios-master | Memory health |
| `brownfield-compatibility-checklist.md` | @architect | Legacy assessment |
| `issue-triage-checklist.md` | @pm | GitHub issue triage |

---

## MCP & Tool Priority

### Native First (ALWAYS)

| Task | Tool |
|------|------|
| Read files | `Read` |
| Write/edit files | `Write` / `Edit` |
| Run commands | `Bash` |
| Search files | `Glob` |
| Search content | `Grep` |

### MCP Servers

| MCP | Purpose | Access |
|-----|---------|--------|
| Context7 | Library docs | `resolve-library-id` → `get-library-docs` |
| EXA | Web search | `web_search_exa` |
| Apify | Web scraping | `search-actors` → `call-actor` |
| playwright | Browser | Direct MCP |
| CodeRabbit | Code review | WSL execution |

---

## Handoff Protocol Details

### When Switching Agents

1. **Outgoing** generates YAML artifact:
   ```yaml
   handoff:
     from_agent: "{current}"
     to_agent: "{new}"
     story_context:
       story_id: "{id}"
       story_status: "{status}"
       current_task: "{task}"
       branch: "{branch}"
     decisions: [max 5]
     files_modified: [max 10]
     blockers: [max 3]
     next_action: "{suggested command}"
   ```

2. **Incoming** receives: full persona + compact handoff (~379 tokens)
3. **Savings:** ~33% per switch, ~57% after 2 switches
4. **Storage:** `.aios/handoffs/handoff-{from}-to-{to}-{timestamp}.yaml`
5. **Retention:** Max 3 artifacts (oldest discarded on 4th)

---

## Squads Reference

### AIOS Core Squad
11 agents for full-stack development. Activate: `/AIOS:agents:{name}`

### AFS (AIOS Forge Squad)
Framework evolution. 7 agents. Activate: `/SQUADS:afs:{name}`
- **aios-catalyst (Nova):** Performance optimization, token reduction
- **aios-nexus:** Integration and connection
- **aios-architect (Athena):** Framework architecture
- **aios-oracle:** Coordination and routing
- **aios-sentinel (Vigil):** Quality validation
- **aios-forge (Vulcan):** Component creation
- **aios-scout:** Discovery and research

### Ultimate Landing Page Squad
Landing page creation. 9 agents. Activate: `/SQUADS:ultimate-lp:{name}`

### BrandCraft Squad
Branding and identity. 8 agents. Activate: `/SQUADS:brandcraft:{name}`

### NSC (Nirvana Squad Creator)
Meta-squad for creating new squads. 9 agents. Activate: `/SQUADS:nsc:{name}`

---

## CLI Commands

```bash
aios doctor              # Full health diagnostic + governance checks
aios graph --deps        # Dependency tree (ASCII/JSON/HTML/Mermaid/DOT)
aios graph --stats       # Entity statistics and cache metrics
aios graph --watch       # Live reload mode
```

## Configuration

| File | Purpose |
|------|---------|
| `core-config.yaml` | Framework config, boundary protection toggle |
| `.claude/settings.json` | IDE-level deny/allow rules |
| `.env` | Environment variables |
| `aios.config.js` | Project-specific settings |
