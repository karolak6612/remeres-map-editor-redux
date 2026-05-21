# ADR — Rust Backend Rewrite and TypeScript/React Shell Direction

## Status

Proposed.

## Context

NoctMapEditor currently starts from a C++/WxWidgets/OpenGL lineage through Remere's Map Editor Redux. Redux is valuable as a modernization and behavior reference, but the desired long-term direction is a Rust-owned backend with a frontend that is easier for Claude/design tooling to iterate on.

A TypeScript/React shell can make UI design, layout, dialogs, and dock surfaces easier to evolve, but map/editor truth must not move into the frontend.

## Decision

NoctMapEditor will target this long-term split:

```text
Rust backend = source of truth
TypeScript/React shell = frontend projection and command surface
Tauri = desktop bridge
C++/Wx surface = legacy reference and temporary comparison target
```

Rust owns:

- map model
- tile/item mutation
- command stack
- undo/redo
- OTBM read/write
- XML sidecars
- OTB/items metadata
- DAT/SPR/assets loading
- ClientID/ServerID mapping
- brush catalog
- autoborder
- renderer frame planning
- minimap
- scripting command API
- diagnostics

React owns:

- docks
- menus
- dialogs
- panels
- layout persistence
- filters/search UI
- visual design system
- command invocation wrappers
- frontend-only transient UI state

Tauri owns:

- command transport
- backend events
- desktop lifecycle integration
- secure frontend/backend boundary

## Consequences

### Positive

- Backend behavior becomes testable independent of UI.
- UI can be redesigned without risking map corruption.
- Claude/design agents can iterate React components more safely.
- Tauri command/event DTOs create clear frontend/backend contracts.
- The C++ source remains a reference instead of a permanent implementation dependency.

### Negative

- The rewrite must avoid big-bang migration.
- There will be duplicate concepts during transition.
- DTO/schema drift must be managed.
- Tauri adds packaging and build complexity.
- Rust APIs need stricter stability before UI migration.

## Non-goals

- Do not rewrite the whole editor in one PR.
- Do not move map state into React.
- Do not copy Redux line-by-line without SPEC/HARNESS.
- Do not delete useful C++ behavior before Rust parity exists.
- Do not implement modern Canary assets before a compatibility matrix exists.

## Required proof before React/Tauri becomes primary

React/Tauri can become the main shell only after these are proven:

- New Map works.
- Open Map hydrates Rust map state.
- Save/Save As roundtrip safely.
- Client asset runtime loads legacy client roots.
- Item/brush palette works from Rust projections.
- Basic brush placement dispatches Rust commands.
- Canvas renders from Rust frame/snapshot protocol.
- Preferences can select client version/root.
- Diagnostics expose asset/map/version problems.
- Existing C++/Redux behavior has documented parity or intentional divergence.

## Implementation policy

Each migration step must be a GSD/AIOX slice with:

- `S##-PLAN.md`
- `S##-SPEC.md`
- `S##-HARNESS.md`
- RED test
- GREEN implementation
- `S##-SUMMARY.md`
- `caveman-review` before PR

## Preferred first implementation after docs

Start with a minimal command/event proof, not the full canvas:

```text
Rust build_info command
  -> Tauri invoke
  -> React DiagnosticPanel
  -> TypeScript test with mocked invoke
  -> Rust command test
```

Then:

```text
Brush catalog projection
  -> React PaletteDock
  -> select brush
  -> dispatch Rust SetTileGround command
  -> receive tiles-changed event
```

## Open questions

- Should the core crate be named `noct_core`, `rme_core`, or `noctmap_core`?
- Should TypeScript DTOs be generated from Rust using `ts-rs`, or manually maintained initially?
- Should the React canvas consume a Rust-rendered bitmap first, or embed a native/wgpu surface?
- Which Canary asset support level is required before React shell becomes default?
- How long should the C++/Wx implementation remain buildable during rewrite?
