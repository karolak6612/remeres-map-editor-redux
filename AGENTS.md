# AGENTS.md - Synkra AIOX (Codex CLI)

Este arquivo define as instrucoes do projeto para o Codex CLI.

<!-- AIOX-MANAGED-START: core -->
## Core Rules

1. Siga a Constitution em `.aiox-core/constitution.md`
2. Priorize `CLI First -> Observability Second -> UI Third`
3. Trabalhe por stories em `docs/stories/`
4. Nao invente requisitos fora dos artefatos existentes
<!-- AIOX-MANAGED-END: core -->

<!-- AIOX-MANAGED-START: quality -->
## Quality Gates

- Rode `npm run lint`
- Rode `npm run typecheck`
- Rode `npm test`
- Atualize checklist e file list da story antes de concluir
<!-- AIOX-MANAGED-END: quality -->

<!-- AIOX-MANAGED-START: codebase -->
## Project Map

- Core framework: `.aiox-core/`
- CLI entrypoints: `bin/`
- Shared packages: `packages/`
- Tests: `tests/`
- Docs: `docs/`
<!-- AIOX-MANAGED-END: codebase -->

<!-- AIOX-MANAGED-START: commands -->
## Common Commands

- `npm run sync:ide`
- `npm run sync:ide:check`
- `npm run sync:skills:codex`
- `npm run sync:skills:codex:global` (opcional; neste repo o padrao e local-first)
- `npm run validate:structure`
- `npm run validate:agents`
<!-- AIOX-MANAGED-END: commands -->

<!-- AIOX-MANAGED-START: shortcuts -->
## Agent Shortcuts

Preferencia de ativacao no Codex CLI:
1. Use `/skills` e selecione `aiox-<agent-id>` vindo de `.codex/skills` (ex.: `aiox-architect`)
2. Se preferir, use os atalhos abaixo (`@architect`, `/architect`, etc.)

Interprete os atalhos abaixo carregando o arquivo correspondente em `.aiox-core/development/agents/` (fallback: `.codex/agents/`), renderize o greeting via `generate-greeting.js` e assuma a persona ate `*exit`:

- `@architect`, `/architect`, `/architect.md` -> `.aiox-core/development/agents/architect.md`
- `@dev`, `/dev`, `/dev.md` -> `.aiox-core/development/agents/dev.md`
- `@qa`, `/qa`, `/qa.md` -> `.aiox-core/development/agents/qa.md`
- `@pm`, `/pm`, `/pm.md` -> `.aiox-core/development/agents/pm.md`
- `@po`, `/po`, `/po.md` -> `.aiox-core/development/agents/po.md`
- `@sm`, `/sm`, `/sm.md` -> `.aiox-core/development/agents/sm.md`
- `@analyst`, `/analyst`, `/analyst.md` -> `.aiox-core/development/agents/analyst.md`
- `@devops`, `/devops`, `/devops.md` -> `.aiox-core/development/agents/devops.md`
- `@data-engineer`, `/data-engineer`, `/data-engineer.md` -> `.aiox-core/development/agents/data-engineer.md`
- `@ux-design-expert`, `/ux-design-expert`, `/ux-design-expert.md` -> `.aiox-core/development/agents/ux-design-expert.md`
- `@squad-creator`, `/squad-creator`, `/squad-creator.md` -> `.aiox-core/development/agents/squad-creator.md`
- `@aiox-master`, `/aiox-master`, `/aiox-master.md` -> `.aiox-core/development/agents/aiox-master.md`
<!-- AIOX-MANAGED-END: shortcuts -->


---

## Available Skills

| Skill | Path | Description |
|-------|------|-------------|
| `aios-god-mode` | `.codex/skills/aios-god-mode/SKILL.md` | Synkra AIOS God Mode — loads aiox-god-mode skill |
| `aiox-analyst` | `.codex/skills/aiox-analyst/SKILL.md` | Business Analyst (Atlas). Use for market research, competitive analysis, user research, brainstorming session facilitation, structured ideation workshops, feasibility studies, i... |
| `aiox-architect` | `.codex/skills/aiox-architect/SKILL.md` | Architect (Aria). Use for system architecture (fullstack, backend, frontend, infrastructure), technology stack selection (technical evaluation), API design (REST/GraphQL/tRPC/We... |
| `aiox-data-engineer` | `.codex/skills/aiox-data-engineer/SKILL.md` | Database Architect & Operations Engineer (Dara). Use for database design, schema architecture, Supabase configuration, RLS policies, migrations, query optimization, data modelin... |
| `aiox-dev` | `.codex/skills/aiox-dev/SKILL.md` | Full Stack Developer (Dex). Use for code implementation, debugging, refactoring, and development best practices |
| `aiox-devops` | `.codex/skills/aiox-devops/SKILL.md` | GitHub Repository Manager & DevOps Specialist (Gage). Use for repository operations, version management, CI/CD, quality gates, and GitHub push operations. ONLY agent authorized... |
| `aiox-god-mode` | `.codex/skills/aiox-god-mode/SKILL.md` | The Supreme AIOX Operator — creates, configures, and orchestrates everything in Synkra AIOX. Creates agents, tasks, workflows, squads, templates, checklists, rules, and data files. Operates all 11 agents, 207+ tasks, 15 workflows. Enforces Constitutional governance, story lifecycle, and delegation matrix. Activates when users mention AIOX, agents, stories, epics, workflows, sprints, quality gates, creating components, or any development orchestration task. |
| `aiox-master` | `.codex/skills/aiox-master/SKILL.md` | AIOX Master Orchestrator & Framework Developer (Orion). Use when you need comprehensive expertise across all domains, framework component creation/modification, workflow orchest... |
| `aiox-pm` | `.codex/skills/aiox-pm/SKILL.md` | Product Manager (Morgan). Use for PRD creation (greenfield and brownfield), epic creation and management, product strategy and vision, feature prioritization (MoSCoW, RICE), roa... |
| `aiox-po` | `.codex/skills/aiox-po/SKILL.md` | Product Owner (Pax). Use for backlog management, story refinement, acceptance criteria, sprint planning, and prioritization decisions |
| `aiox-qa` | `.codex/skills/aiox-qa/SKILL.md` | Test Architect & Quality Advisor (Quinn). Use for comprehensive test architecture review, quality gate decisions, and code improvement. Provides thorough analysis including requ... |
| `aiox-sm` | `.codex/skills/aiox-sm/SKILL.md` | Scrum Master (River). Use for user story creation from PRD, story validation and completeness checking, acceptance criteria definition, story refinement, sprint planning, backlo... |
| `aiox-squad-creator` | `.codex/skills/aiox-squad-creator/SKILL.md` | Squad Creator (Craft). Use to create, validate, publish and manage squads |
| `aiox-ux-design-expert` | `.codex/skills/aiox-ux-design-expert/SKILL.md` | UX/UI Designer & Design System Architect (Uma). Complete design workflow - user research, wireframes, design systems, token extraction, component building, and quality assurance |
| `architect-first` | `.codex/skills/architect-first/SKILL.md` | Guide for implementing the Architect-First development philosophy - perfect architecture, pragmatic execution, quality guaranteed by tests. Use this skill when starting new features, refactoring systems, or when architectural decisions are needed. Enforces non-negotiables like complete design/documentation before code, zero coupling, and validation by multiple perspectives before structural decisions. |
| `checklist-runner` | `.codex/skills/checklist-runner/SKILL.md` | | |
| `coderabbit-review` | `.codex/skills/coderabbit-review/SKILL.md` | | |
| `mcp-builder` | `.codex/skills/mcp-builder/SKILL.md` | Guide for creating high-quality MCP (Model Context Protocol) servers that enable LLMs to interact with external services through well-designed tools. Use when building MCP servers to integrate external APIs or services, whether in Python (FastMCP) or Node/TypeScript (MCP SDK). |
| `skill-creator` | `.codex/skills/skill-creator/SKILL.md` | Guide for creating effective skills. This skill should be used when users want to create a new skill (or update an existing skill) that extends Claude's capabilities with specialized knowledge, workflows, or tool integrations. |
| `synapse` | `.codex/skills/synapse/SKILL.md` | This skill should be used when users want to understand the SYNAPSE context engine, manage domains, configure context rules, or troubleshoot rule injection. Use when asked about SYNAPSE architecture, domain management, star-commands, context brackets, or the 8-layer processing pipeline. |
| `tech-search` | `.codex/skills/tech-search/SKILL.md` | | |

To use a skill, read the SKILL.md file at the path indicated above.
