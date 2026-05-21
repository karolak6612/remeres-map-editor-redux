# Redux / Canary Authority Matrix

## Purpose

This document tells Claude Code, Codex, and future agents which upstream source should be treated as authority for each NoctMapEditor rewrite area.

The goal is to avoid vague prompts like "rewrite RME in Rust". Every implementation slice must name the upstream authority, divergence policy, and harness family.

## Authority levels

| Level | Meaning |
|---|---|
| P0 | Must preserve behavior unless explicit divergence is approved. |
| P1 | Strong reference; preserve observable behavior where practical. |
| P2 | Research reference; do not copy without validation. |
| P3 | Inspiration only. |

## Upstream sources

| Source | Use |
|---|---|
| `karolak6612/remeres-map-editor-redux` | Primary modernization and behavior reference for Redux architecture, renderer, UI parity, brush systems, and known WIP gaps. |
| Original/legacy Remere's Map Editor lineage | Reference for long-standing RME behavior and OTBM compatibility expectations. |
| `opentibiabr/remeres-map-editor` | Canary-compatible RME reference, especially client assets and modern compatibility. |
| `opentibiabr/canary` | Server/runtime truth for Canary item/client behavior. |
| OpenTibiaBR docs | User-facing Canary/RME compatibility notes. |
| Community reports/OTLand | P2/P3 diagnostics only; never final authority without source-backed proof. |

## Behavior matrix

| Area | Authority | Level | Rewrite policy | Harness family |
|---|---:|---:|---|---|
| OTBM load/save | Legacy RME + Redux | P0 | Rust must roundtrip safely. Unknown data must be preserved or diagnosed. | OTBM roundtrip |
| OTBM version detection | Legacy RME + Redux | P0 | Detect before full load when needed for client/runtime choice. | Rust core |
| ServerID/ClientID mapping | Canary/RME + OTB metadata | P0 | Mapping must be explicit, testable, and diagnostic-rich. | Asset pipeline |
| XML sidecars | Legacy RME | P0 | Preserve houses, spawns, creatures, waypoints naming and attributes. | Roundtrip safety |
| DAT/SPR legacy clients | Legacy RME + Redux | P0 | Format-specific parsing; reject unsupported signatures honestly. | Asset pipeline |
| Canary assets/appearances | OpenTibiaBR RME + Canary | P0/P1 | Research first; no fake DAT/SPR assumptions. | Compatibility matrix |
| Item metadata | OTB/items.xml + Canary | P0 | Merge deterministic metadata and keep id mapping clear. | Rust core + asset pipeline |
| Brush catalog | Redux + legacy brush XML | P1 | Load from real config/XML; hardcoded brush list only as bootstrap fixture. | Rust core |
| Palette/tilesets | Redux + legacy UI/config | P1 | Preserve user-visible grouping and ordering where practical. | React/Tauri + Rust projection |
| Autoborder | Redux + legacy RME | P1 | Golden fixtures required before large algorithm work. | Rust core |
| Map canvas interactions | Redux/legacy MapCanvas | P1 | Preserve observable editing gestures; document divergence. | UI + renderer |
| Renderer batching | Redux renderer | P1 | Rust/wgpu can diverge internally; output and performance must be tested. | Renderer benchmark/smoke |
| Lighting/minimap | Redux | P1/P2 | Implement after base map/assets are stable. | Renderer |
| Search/replace | Redux + legacy RME | P1 | Preserve observable workflows; backend can be Rust-native. | Rust core + UI |
| Import/export/merge | Legacy RME + Redux | P1 | Preserve data safety over exact UI behavior. | Roundtrip/import harness |
| Lua/scripting | Redux if available + Noct policy | P2 | Prefer safe command API over raw internals. | Sandbox tests |
| Modern UI design | Noct/React design system | P3 | UX may improve; backend contracts must remain stable. | React tests |

## Divergence rules

Divergence is allowed only when:

1. Redux documents the behavior as WIP/buggy.
2. Legacy behavior is unsafe or corrupts data.
3. Canary compatibility requires different behavior.
4. React/Tauri UI needs a different presentation but same backend contract.
5. The SPEC names the divergence and HARNESS proves the new behavior.

## Research requirements by slice

### OTBM slice

Must inspect:

- OTBM root/header code.
- MAP_DATA parsing.
- tile area parsing.
- tile/item attribute parsing.
- save/serialize logic.
- version/client mapping behavior.

Must produce:

- fixture list.
- equality rules.
- unknown-data policy.
- corruption behavior.

### Asset slice

Must inspect:

- DAT parser behavior.
- SPR parser behavior.
- OTB parser behavior.
- items.xml merge behavior.
- Canary asset format expectations.

Must produce:

- client version matrix.
- file layout matrix.
- unsupported diagnostics.
- id mapping tests.

### Brush/autoborder slice

Must inspect:

- brush XML/config files.
- brush manager/source code.
- terrain/autoborder rules.
- UI palette grouping.

Must produce:

- minimal XML fixtures.
- duplicate validation tests.
- golden terrain transition tests.

### React/Tauri slice

Must inspect:

- current desired UI behavior.
- backend command/event contract.
- design token strategy.

Must produce:

- TypeScript DTOs.
- fake backend fixture.
- real invoke smoke test.
- no durable map state in React.

## Prompt for authority research

```text
RESEARCH the upstream authority for this NoctMapEditor slice.

Slice area: <OTBM | assets | brush | renderer | UI | scripting>

Use Tavily and GitHub source inspection.
Prioritize:
- karolak6612/remeres-map-editor-redux
- opentibiabr/remeres-map-editor
- opentibiabr/canary
- OpenTibiaBR docs

Output:
- exact files/source paths
- behavior contracts
- known bugs/WIP notes
- required fixtures
- recommended SPEC acceptance criteria
- recommended HARNESS commands

Do not implement.
```

## Minimum acceptable citation style in SPEC

Every SPEC should include a section like:

```md
## Legacy authority

- Redux: `source/path/file.cpp::FunctionName` — behavior summary.
- Config: `brushes/...xml` — data contract summary.
- Canary/OpenTibiaBR: source path or docs URL — compatibility reason.

## Intentional divergence

- <none>, or exact divergence and reason.
```

## Safety reminder

If the source path cannot be found, the slice is research-blocked, not implementation-ready.
