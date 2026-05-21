# GSD / AIOX Rewrite Task Factory

## Purpose

This file gives Claude Code, Codex, AIOX God Mode, Superpowers, Caveman, and RTK a repeatable way to convert the Rust backend rewrite roadmap into safe milestones, slices, and tasks.

Use it when creating implementation work from the documentation roadmap.

## Mandatory stack

For every product-behavior implementation slice:

```text
AIOX/God Mode
  -> Superpowers workflow selection
  -> GSD milestone/slice routing
  -> SPEC
  -> HARNESS
  -> RED test
  -> implementation
  -> GREEN test
  -> rme-quality-gate or equivalent review
  -> caveman-review
  -> summary
  -> PR
```

## Session boot prompt

Use this at the beginning of a Claude Code session:

```text
/aiox-god-mode

OPERATE noctmapeditor as a durable GSD loop.

Read first:
- AGENTS.md if present
- CLAUDE.md if present
- README.md
- active GSD state if present
- docs/ai/NOCTMAPEDITOR_RUST_REWRITE_MASTERPLAN.md
- docs/ai/TS_REACT_TAURI_FRONTEND_STRATEGY.md
- docs/ai/RUST_BACKEND_REWRITE_ADR.md

Use:
- superpowers:using-superpowers
- superpowers:using-git-worktrees before implementation
- superpowers:test-driven-development for implementation
- superpowers:subagent-driven-development for multi-domain slices
- Context7 for library/API uncertainty
- Tavily for public Redux/Canary/RME research
- caveman-review before commit/PR
- rtk for high-volume output

Do not implement from README prose alone.
Do not broaden scope.
Do not stub.
Do not update roadmap/status before evidence exists.
```

## Task creation prompt

Use this to create a new slice:

```text
CREATE a new GSD slice for noctmapeditor.

Target milestone: M###-name
Target behavior: <one narrow behavior>
Legacy authority: <Redux/RME/Canary source paths or explicit research needed>
Primary files/seams: <target files only>
Non-goals: <what must not be changed>
Harness family: <Rust core | Tauri shell | React UI | Asset pipeline | Roundtrip safety | Renderer | Manual smoke>

Required artifacts:
- S##-PLAN.md
- S##-SPEC.md
- S##-HARNESS.md

Definition of ready:
- SPEC has observable behavior or backend contract.
- Legacy authority or deliberate divergence is named.
- HARNESS contains exact RED and GREEN commands.
- Target files are narrow.
- Baseline failures are separated.

Stop after writing plan/spec/harness. Do not implement until ready is confirmed.
```

## Implementation prompt

Use this after SPEC/HARNESS exists:

```text
IMPLEMENT the active GSD slice only.

Rules:
- Use superpowers:test-driven-development.
- Start with the failing harness.
- Implement the smallest code change.
- Prefer Rust domain changes over bridge/UI work when state ownership is involved.
- Keep React as projection only.
- Run exact verification from S##-HARNESS.md.
- Write S##-SUMMARY.md with exact evidence.
- Run caveman-review and fix every gap before PR.

Do not touch unrelated milestones or docs unless required by this slice.
```

## Research prompt

Use this for Redux/Canary research slices:

```text
RESEARCH only. Do not implement.

Use Tavily for public research and GitHub source inspection:
- karolak6612/remeres-map-editor-redux
- opentibiabr/remeres-map-editor
- opentibiabr/canary
- OpenTibiaBR docs
- OTBM/DAT/SPR/OTB/appearances compatibility notes

Use Context7 only for library/API docs:
- Rust
- Tauri
- React
- quick-xml
- wgpu
- mlua
- test frameworks

Output:
- exact source paths
- behavior contracts
- known bugs/divergences
- recommended implementation slices
- harness ideas

Record findings in the slice SUMMARY and durable docs if needed.
```

## Slice templates

### Rust core slice

```text
S##: <domain behavior>

SPEC:
- Observable behavior:
- Legacy authority:
- Current gap:
- Acceptance criteria:
- Non-goals:
- Target files:
- Regression risks:

HARNESS:
- RED command:
- GREEN command:
- Fixture/data:
- Baseline failures:
- Evidence to paste:
```

### Tauri/React slice

```text
S##: <frontend bridge behavior>

SPEC:
- Rust command/event contract:
- TypeScript DTO:
- React component responsibility:
- Mock/fake backend behavior:
- Non-goals:

HARNESS:
- Rust command test
- TypeScript unit test
- Playwright or component test if visual
- mocked invoke test
```

### Asset pipeline slice

```text
S##: <asset/client behavior>

SPEC:
- Client family:
- Files involved:
- ID mapping policy:
- Unsupported behavior policy:
- Diagnostics:

HARNESS:
- synthetic fixture
- real-client smoke if available
- no-corruption assertion
- diagnostic assertion
```

### OTBM roundtrip slice

```text
S##: <roundtrip behavior>

SPEC:
- Input fixture:
- Load expectations:
- Save expectations:
- Load-again equality:
- Opaque preservation:
- Mapper behavior:

HARNESS:
- cargo test focused module
- fixture path
- equality/diff output
- corruption failure case
```

## Suggested first task queue

### Documentation-only queue

- [x] Create Rust rewrite masterplan.
- [x] Create TS/React/Tauri strategy.
- [x] Create Rust backend rewrite ADR.
- [ ] Create Redux/Canary authority matrix.
- [ ] Create AGENTS.md / CLAUDE.md repo instructions if missing.

### First implementation queue

- [ ] Create Cargo workspace.
- [ ] Add `noct_core` crate with `MapPosition`, `Item`, `Tile`, and `MapModel`.
- [ ] Add dirty/generation invariants.
- [ ] Add command stack domain contract.
- [ ] Add OTBM header parser.
- [ ] Add load/save/load fixture harness.

### First React/Tauri queue

- [ ] Add `apps/desktop-tauri` skeleton.
- [ ] Add minimal Rust Tauri command returning build info.
- [ ] Add TypeScript DTOs for diagnostics and commands.
- [ ] Add React `AppShell` and `DiagnosticPanel`.
- [ ] Add fake backend for UI tests.
- [ ] Add first command invoke test.

## RTK usage requirements

Use `rtk` for:

```bash
rtk git status --short --branch
rtk git diff --check
rtk git diff --stat
rtk cargo test --workspace
rtk cargo clippy --workspace --all-targets --all-features -- -D warnings
rtk npm test
rtk npm run build
```

Keep exact failure text. Summarize noisy success output.

## Caveman usage

Use Caveman for:

- large closeout summaries
- terse review output
- reducing context bloat
- final gap review before PR

Required before PR:

```text
caveman-review
```

If Caveman finds any gap, fix it before PR. Do not document around it.

## Branch naming

Documentation branch:

```text
docs/rust-backend-ts-react-rewrite-roadmap
```

Implementation branches:

```text
gsd/M###/S##-short-topic
```

Commit examples:

```text
docs(M001/S01): add Rust backend rewrite masterplan
feat(M002/S02): add Rust map primitives
test(M003/S03): add OTBM load-save-load roundtrip fixtures
```

## PR body template

```md
## Summary
- 

## SPEC/HARNESS
- SPEC:
- HARNESS:

## Verification
- [ ] `rtk git diff --check`
- [ ] focused tests:
- [ ] `caveman-review`

## Known gaps
- 

## Next safe step
- 
```
