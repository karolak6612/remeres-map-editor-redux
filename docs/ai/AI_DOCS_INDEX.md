# AI Documentation Index — NoctMapEditor

## Purpose

This index is the entry point for Claude Code, Codex, AIOX, and human maintainers working on the Rust backend rewrite and future TypeScript/React/Tauri shell.

Read these files before creating implementation tasks.

## Core docs

1. `docs/ai/NOCTMAPEDITOR_RUST_REWRITE_MASTERPLAN.md`
   - Full backend rewrite roadmap.
   - Complete task universe.
   - Suggested milestone order.

2. `docs/ai/TS_REACT_TAURI_FRONTEND_STRATEGY.md`
   - Frontend strategy.
   - React/Tauri ownership boundaries.
   - First UI prototype slices.

3. `docs/ai/RUST_BACKEND_REWRITE_ADR.md`
   - Architectural decision record.
   - Rust as source of truth.
   - React as projection and design shell.

4. `docs/ai/GSD_AIOX_REWRITE_TASK_FACTORY.md`
   - Prompts for creating GSD/AIOX tasks.
   - SPEC/HARNESS templates.
   - RTK/Caveman/Superpowers workflow.

## Recommended reading order for Claude Code

```text
README.md
AGENTS.md if present
CLAUDE.md if present
docs/ai/AI_DOCS_INDEX.md
docs/ai/NOCTMAPEDITOR_RUST_REWRITE_MASTERPLAN.md
docs/ai/RUST_BACKEND_REWRITE_ADR.md
docs/ai/TS_REACT_TAURI_FRONTEND_STRATEGY.md
docs/ai/GSD_AIOX_REWRITE_TASK_FACTORY.md
```

## Immediate next documentation tasks

- [ ] Add `AGENTS.md` with repo-specific execution contract.
- [ ] Add `CLAUDE.md` for Claude Code memory.
- [ ] Add `.mcp.json` with Context7 and Tavily.
- [ ] Add Redux/Canary authority matrix.
- [ ] Add first GSD milestone scaffold for documentation foundation.

## Immediate next implementation tasks

Do not start implementation until the documentation foundation PR is reviewed.

After review:

1. Create Cargo workspace.
2. Add `noct_core` crate.
3. Implement `MapPosition`, `Item`, `Tile`, `MapModel`.
4. Add dirty/generation tests.
5. Add first Tauri build-info command proof.

## Safety reminder

No broad rewrites. Every behavior change needs a narrow SPEC, HARNESS, failing test, implementation, verification, and review.
