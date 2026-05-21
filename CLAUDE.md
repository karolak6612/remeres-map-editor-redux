# CLAUDE.md — NoctMapEditor

Read `AGENTS.md` first. It is the canonical repository execution contract.

## Project mission

NoctMapEditor is being planned as a Rust-owned backend rewrite of Remere's Map Editor / Redux with a future TypeScript/React/Tauri shell.

Do not treat this as a normal UI redesign. The backend rewrite must be data-safe, testable, and guided by upstream RME/Redux/Canary behavior.

## Session boot

At the start of a session, inspect:

```bash
rtk git status --short --branch
rtk git diff --stat
```

Then read:

```text
README.md
AGENTS.md
docs/ai/AI_DOCS_INDEX.md
docs/ai/NOCTMAPEDITOR_RUST_REWRITE_MASTERPLAN.md
docs/ai/RUST_BACKEND_REWRITE_ADR.md
docs/ai/REDUX_CANARY_AUTHORITY_MATRIX.md
docs/ai/TS_REACT_TAURI_FRONTEND_STRATEGY.md
docs/ai/GSD_AIOX_REWRITE_TASK_FACTORY.md
```

If GSD state exists, read it before planning or editing.

## Required workflow

Use this for implementation work:

1. `superpowers:using-superpowers`
2. `superpowers:using-git-worktrees`
3. AIOX/GSD routing
4. `superpowers:test-driven-development`
5. Context7 for library/API details
6. Tavily for public Redux/RME/Canary research
7. RTK for high-volume output
8. Caveman review before PR

## Context7 and Tavily policy

Use Context7 for:

- Rust crates
- Tauri APIs
- React/TypeScript APIs
- quick-xml
- wgpu
- testing libraries

Use Tavily for:

- Remere's Map Editor Redux research
- Canary compatibility
- OpenTibiaBR RME docs
- OTBM/DAT/SPR/OTB/appearances research
- public issues/releases/discussions

Do not use memory for uncertain library API details.

## Rewrite safety

Never:

- implement from README prose alone;
- move durable map state into React;
- fake OTBM or Canary support;
- silently drop unknown binary data;
- create broad architecture without a narrow SPEC/HARNESS;
- claim parity without source path and test evidence.

## Preferred next actions

For this documentation branch:

1. Review the docs in `docs/ai/`.
2. Create a docs PR.
3. After review, start a new implementation branch for a Cargo workspace skeleton.

For first implementation branch:

1. Create Rust workspace.
2. Add `noct_core` crate.
3. Implement `MapPosition`, `Item`, `Tile`, `MapModel`.
4. Add dirty/generation tests.
5. Add first Tauri build-info command proof only after core skeleton is stable.
