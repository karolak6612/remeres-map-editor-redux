# AGENTS.md - Synkra AIOX / NoctMapEditor

Este arquivo define as instrucoes do projeto para Codex CLI, Claude Code e agentes AIOX.

<!-- AIOX-MANAGED-START: core -->
## Core Rules

1. Siga a Constitution em `.aiox-core/constitution.md` quando existir.
2. Priorize `CLI First -> Observability Second -> UI Third`.
3. Trabalhe por stories em `docs/stories/` quando o fluxo AIOX estiver ativo.
4. Nao invente requisitos fora dos artefatos existentes.
<!-- AIOX-MANAGED-END: core -->

<!-- AIOX-MANAGED-START: quality -->
## Quality Gates

- Rode `npm run lint` quando existir.
- Rode `npm run typecheck` quando existir.
- Rode `npm test` quando existir.
- Atualize checklist e file list da story antes de concluir.
<!-- AIOX-MANAGED-END: quality -->

<!-- AIOX-MANAGED-START: codebase -->
## Project Map

- Core framework: `.aiox-core/`
- CLI entrypoints: `bin/`
- Shared packages: `packages/`
- Tests: `tests/`
- Docs: `docs/`
- AI rewrite docs: `docs/ai/`
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

## NoctMapEditor Rust Rewrite Mission

NoctMapEditor is being planned as a Rust-owned backend rewrite of Remere's Map Editor / Redux with a future TypeScript/React/Tauri shell.

Do not perform broad rewrites. Every implementation must be routed through a narrow SPEC/HARNESS slice with verifiable evidence.

## Required Reading

At the start of every rewrite session, read:

```text
README.md
docs/ai/AI_DOCS_INDEX.md
docs/ai/NOCTMAPEDITOR_RUST_REWRITE_MASTERPLAN.md
docs/ai/RUST_BACKEND_REWRITE_ADR.md
docs/ai/REDUX_CANARY_AUTHORITY_MATRIX.md
docs/ai/TS_REACT_TAURI_FRONTEND_STRATEGY.md
docs/ai/GSD_AIOX_REWRITE_TASK_FACTORY.md
```

If GSD files exist, also read active state, milestone context, slice plan, SPEC, and HARNESS before touching code.

## Rewrite Workflow

Use this order for product behavior:

1. Confirm branch/worktree cleanliness.
2. Read docs and active slice state.
3. Use Superpowers workflow selection.
4. Use a clean branch/worktree.
5. Write or update SPEC/HARNESS.
6. Add RED test.
7. Implement smallest change.
8. Run focused verification.
9. Run broader relevant checks.
10. Write SUMMARY evidence.
11. Run `caveman-review` before PR.

## Source Authority

Primary sources:

- `karolak6612/remeres-map-editor-redux`
- `opentibiabr/remeres-map-editor`
- `opentibiabr/canary`
- OpenTibiaBR docs

Every parity slice must cite exact source paths or state that research is incomplete.

## Tool Policy

Use:

- AIOX/God Mode for routing.
- Superpowers for workflow discipline.
- Context7 for library/API docs.
- Tavily for public Redux/RME/Canary research.
- RTK for high-volume command output.
- Caveman for review and compact closeout.

## Safety Rules

Never:

- implement from README prose alone;
- start a broad rewrite without SPEC/HARNESS;
- move durable map state into React;
- silently drop unknown OTBM data;
- fake Canary asset support;
- stub a behavior and claim completion;
- update state/roadmap before evidence exists;
- mix unrelated backend/frontend milestones in one PR.

## Preferred Branch Names

Documentation:

```text
docs/rust-backend-ts-react-rewrite-roadmap
```

Implementation:

```text
gsd/M###/S##-short-topic
```

## Verification Examples

Use whatever is relevant to the slice:

```bash
rtk git status --short --branch
rtk git diff --check
rtk cargo test --workspace
rtk cargo clippy --workspace --all-targets --all-features -- -D warnings
rtk npm test
rtk npm run build
```

If the command does not exist yet because the workspace is not bootstrapped, record that honestly in the SUMMARY and do not claim it passed.

---

## Available Skills

| Skill | Path | Description |
|-------|------|-------------|
| `aios-god-mode` | `.codex/skills/aios-god-mode/SKILL.md` | Synkra AIOS God Mode — loads aiox-god-mode skill |
| `aiox-analyst` | `.codex/skills/aiox-analyst/SKILL.md` | Business Analyst (Atlas). Use for market research, competitive analysis, user research, brainstorming session facilitation, structured ideation workshops, feasibility studies. |
| `aiox-architect` | `.codex/skills/aiox-architect/SKILL.md` | Architect (Aria). Use for system architecture, backend/frontend/infrastructure, API design, and rewrite architecture. |
| `aiox-dev` | `.codex/skills/aiox-dev/SKILL.md` | Full Stack Developer (Dex). Use for code implementation, debugging, refactoring, and development best practices. |
| `aiox-devops` | `.codex/skills/aiox-devops/SKILL.md` | GitHub Repository Manager & DevOps Specialist (Gage). Use for repository operations, version management, CI/CD, quality gates, and GitHub push operations. |
| `aiox-god-mode` | `.codex/skills/aiox-god-mode/SKILL.md` | Supreme AIOX Operator. Use for orchestration, agents, tasks, workflows, squads, templates, checklists, rules, and data files. |
| `aiox-master` | `.codex/skills/aiox-master/SKILL.md` | AIOX Master Orchestrator & Framework Developer (Orion). |
| `aiox-pm` | `.codex/skills/aiox-pm/SKILL.md` | Product Manager (Morgan). Use for PRDs, epics, roadmap, prioritization. |
| `aiox-po` | `.codex/skills/aiox-po/SKILL.md` | Product Owner (Pax). Use for backlog and acceptance criteria. |
| `aiox-qa` | `.codex/skills/aiox-qa/SKILL.md` | Test Architect & Quality Advisor (Quinn). |
| `aiox-sm` | `.codex/skills/aiox-sm/SKILL.md` | Scrum Master (River). Use for user story creation and validation. |
| `aiox-squad-creator` | `.codex/skills/aiox-squad-creator/SKILL.md` | Squad Creator (Craft). |
| `aiox-ux-design-expert` | `.codex/skills/aiox-ux-design-expert/SKILL.md` | UX/UI Designer & Design System Architect (Uma). |
| `architect-first` | `.codex/skills/architect-first/SKILL.md` | Architect-first development philosophy. |
| `checklist-runner` | `.codex/skills/checklist-runner/SKILL.md` | Checklist execution. |
| `coderabbit-review` | `.codex/skills/coderabbit-review/SKILL.md` | CodeRabbit review support. |
| `mcp-builder` | `.codex/skills/mcp-builder/SKILL.md` | MCP server creation. |
| `skill-creator` | `.codex/skills/skill-creator/SKILL.md` | Skill creation/update. |
| `synapse` | `.codex/skills/synapse/SKILL.md` | SYNAPSE context engine. |
| `tech-search` | `.codex/skills/tech-search/SKILL.md` | Technical research. |

To use a skill, read the `SKILL.md` file at the path indicated above.
