# NoctMapEditor Rust Backend Rewrite Masterplan

## Purpose

This document defines a documentation-first roadmap for rewriting NoctMapEditor into a Rust-owned backend with a future TypeScript/React frontend shell.

The goal is not to translate C++ line-by-line. The goal is to preserve the important behavior contracts from Remere's Map Editor, Redux, and Canary-compatible workflows through small, testable GSD slices.

## Repository context

This repository currently tracks the Remere's Map Editor: Redux codebase and documentation. Redux itself is WIP and explicitly does not promise perfect 1:1 RME compatibility, but it does treat loading/editing OTBM maps as a compatibility requirement.

NoctMapEditor should use Redux as a behavior and architecture reference, while creating a new Rust backend that is safer, testable, and frontend-neutral.

## North-star architecture

```text
NoctMapEditor
├── apps/
│   └── desktop-tauri/              # future TypeScript/React/Tauri shell
├── crates/
│   ├── noct_core/                  # pure Rust editor/map domain
│   ├── noct_io/                    # OTBM/OTB/DAT/SPR/XML/appearances I/O
│   ├── noct_assets/                # client roots, sprites, item metadata, id mapping
│   ├── noct_brushes/               # brushes, palettes, tilesets, autoborder
│   ├── noct_commands/              # command stack, undo/redo, selection edits
│   ├── noct_render/                # frame planning, minimap, wgpu renderer
│   ├── noct_scripting/             # Lua/plugin/scripting sandbox
│   └── noct_tauri/                 # Tauri commands/events/state
├── packages/
│   ├── ui/                         # React components
│   ├── editor-state/               # TypeScript command wrappers and projections
│   └── design-tokens/              # Claude/design-friendly visual tokens
└── docs/ai/                        # GSD/AIOX/Codex/Claude execution docs
```

## Rewrite principles

1. Rust owns all durable editor state.
2. React owns only layout, panels, dialogs, and transient UI state.
3. Tauri is the bridge, not the source of truth.
4. OTBM roundtrip safety is mandatory.
5. Unknown map data must be preserved or explicitly diagnosed.
6. ServerID/ClientID mapping must be explicit and tested.
7. Legacy DAT/SPR clients and Canary/assets clients need separate compatibility contracts.
8. Brush and autoborder behavior must come from real config/XML sources, not permanent hardcoded defaults.
9. Every implementation needs SPEC, HARNESS, RED test, GREEN test, SUMMARY, and review.
10. No broad rewrites without a narrow failing harness.

## Backend task universe

### A. Rust workspace foundation

- [ ] Create Cargo workspace.
- [ ] Add `noct_core` pure Rust crate.
- [ ] Add `noct_io` for binary/XML formats.
- [ ] Add `noct_assets` for client asset runtime.
- [ ] Add `noct_brushes` for brush/autoborder logic.
- [ ] Add `noct_commands` for undo/redo.
- [ ] Add `noct_render` for frame planning and wgpu.
- [ ] Add `noct_tauri` for Tauri commands/events.
- [ ] Add fixture policy and golden test directory.
- [ ] Add CI for Rust format, clippy, tests, and security checks.

### B. Map domain model

- [ ] Implement `MapPosition`.
- [ ] Implement `Item`.
- [ ] Implement `Tile` with ground + ordered stack.
- [ ] Implement `MapModel` sparse storage.
- [ ] Implement `Town`, `House`, `Spawn`, `Creature`, `Waypoint`.
- [ ] Implement dirty-state and generation counters.
- [ ] Implement map statistics.
- [ ] Implement map diff/equality helpers for roundtrip tests.
- [ ] Implement selection model.
- [ ] Implement editor session metadata separately from map data.

### C. Command stack

- [ ] Define `EditorCommand` enum.
- [ ] Implement apply/revert.
- [ ] Implement transaction grouping.
- [ ] Implement brush stroke batching.
- [ ] Implement undo/redo stack.
- [ ] Implement memory limits.
- [ ] Implement command summaries for UI.
- [ ] Add tests for every command.

### D. OTBM parser/serializer

- [ ] Inventory supported OTBM versions.
- [ ] Parse root/header.
- [ ] Parse MAP_DATA.
- [ ] Parse TILE_AREA.
- [ ] Parse TILE/HOUSETILE.
- [ ] Parse inline item and child item nodes.
- [ ] Parse item attributes: text, desc, action id, unique id, depot id, door id, charges, rune charges.
- [ ] Define unknown attribute policy.
- [ ] Preserve opaque/unknown data where possible.
- [ ] Implement serializer.
- [ ] Add load/save/load roundtrip tests.
- [ ] Add corrupted fixture tests.
- [ ] Add v5/v6 ClientID/ServerID mapping tests.

### E. XML sidecars

- [ ] Houses XML load/save.
- [ ] Spawns XML load/save.
- [ ] Creatures XML load/save.
- [ ] Waypoints XML load/save.
- [ ] Missing sidecar fallback.
- [ ] Malformed sidecar diagnostics.
- [ ] Transactional save with OTBM.

### F. OTB/items metadata

- [ ] Parse items.otb.
- [ ] Parse items.xml.
- [ ] Merge XML and OTB metadata.
- [ ] Build server/client id mapper.
- [ ] Add duplicate/missing item diagnostics.
- [ ] Add item search index.
- [ ] Add item category/group mapping.

### G. Legacy DAT/SPR assets

- [ ] Define DAT format table.
- [ ] Define SPR format table.
- [ ] Detect client signatures.
- [ ] Parse sprite geometry.
- [ ] Decode sprite pixels.
- [ ] Materialize sprite atlas.
- [ ] Add missing sprite diagnostics.
- [ ] Add legacy client root discovery.

### H. Canary/assets compatibility

- [ ] Research Canary asset layout.
- [ ] Research appearances.dat/protobuf requirements.
- [ ] Decide protobuf dependency strategy.
- [ ] Add explicit unsupported diagnostics when modern assets are not implemented.
- [ ] Build compatibility matrix for legacy TFS, Canary, and custom clients.

### I. Brush, palette, tileset

- [ ] Parse brush XML/config files.
- [ ] Parse ground brushes.
- [ ] Parse wall brushes.
- [ ] Parse doodads.
- [ ] Parse raw/item brushes.
- [ ] Parse house/creature/waypoint brushes.
- [ ] Parse tilesets.
- [ ] Preserve palette order.
- [ ] Validate duplicate ids/names.
- [ ] Expose frontend projection schema.

### J. Autoborder

- [ ] Inventory Redux autoborder behavior.
- [ ] Define border rule model.
- [ ] Parse autoborder rules.
- [ ] Implement neighbor masks.
- [ ] Implement preview plan.
- [ ] Implement commit-time border placement.
- [ ] Add golden terrain fixtures.

### K. Import/export/merge

- [ ] Import map with offset.
- [ ] Collision policy.
- [ ] Export selected area.
- [ ] Preserve houses/spawns/waypoints.
- [ ] Add merge diagnostics.

### L. Search/replace/diagnostics

- [ ] Item search backend.
- [ ] Tile search backend.
- [ ] Replace items backend.
- [ ] Similarity finder backend.
- [ ] Missing items report.
- [ ] Invalid house/spawn reports.
- [ ] Asset compatibility report.

### M. Renderer/minimap

- [ ] Frame planning in Rust.
- [ ] Sprite batching.
- [ ] Lighting/shade pass.
- [ ] Animation tick model.
- [ ] Minimap renderer.
- [ ] Screenshot/smoke harness.
- [ ] Bench thresholds.

### N. Scripting/plugins

- [ ] Lua command API.
- [ ] Script sandbox.
- [ ] Execution budget.
- [ ] Script registry.
- [ ] Script diagnostics/logs.
- [ ] Plugin extension points.

### O. Tauri/React bridge

- [ ] Tauri command API.
- [ ] Backend event protocol.
- [ ] TypeScript DTOs.
- [ ] React state projection.
- [ ] Mock backend for UI tests.
- [ ] Design-token package.

## Recommended milestones

### M001 — Documentation foundation

- S01 Rust backend masterplan.
- S02 TS/React/Tauri strategy.
- S03 Rust rewrite ADR.
- S04 Redux/Canary authority matrix.
- S05 GSD/AIOX task factory.

### M002 — Rust workspace skeleton

- S01 Cargo workspace.
- S02 `noct_core` map primitives.
- S03 `noct_io` OTBM header parser.
- S04 test fixture layout.
- S05 CI bootstrap.

### M003 — OTBM roundtrip contract

- S01 minimal OTBM load.
- S02 minimal OTBM save.
- S03 load/save/load equality.
- S04 tile stack attributes.
- S05 unknown data policy.

### M004 — Asset compatibility foundation

- S01 items.xml parser.
- S02 items.otb parser.
- S03 id mapper.
- S04 DAT parser.
- S05 SPR parser.

### M005 — Brush/autoborder foundation

- S01 brush XML loader.
- S02 palette projection.
- S03 ground brush placement.
- S04 wall/doodad placement.
- S05 autoborder golden tests.

### M006 — Tauri/React shell prototype

- S01 Tauri app skeleton.
- S02 React AppShell.
- S03 DiagnosticPanel with Rust build info command.
- S04 PaletteDock projection.
- S05 SetTileGround command proof.

## Definition of complete backend rewrite

The rewrite is complete when:

- Map lifecycle is Rust-owned.
- OTBM + sidecars are roundtrip-safe.
- Asset/client compatibility is deterministic and diagnostic-rich.
- Brush/autoborder/search/replace/import/export are backend-owned.
- Renderer frame planning is Rust-owned.
- React/Tauri can replace the legacy UI without losing backend behavior.
- Every behavior has SPEC/HARNESS evidence.
