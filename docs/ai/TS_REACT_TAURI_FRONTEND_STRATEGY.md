# TypeScript / React / Tauri Frontend Strategy

## Purpose

This document defines the future frontend direction for NoctMapEditor: Rust backend, TypeScript/React UI, and Tauri as the desktop bridge.

The backend must stay in Rust. TypeScript/React is for UI composition, design iteration, docks, dialogs, panels, and frontend-only state.

## Target stack

```text
Desktop shell: Tauri 2
Frontend: React + TypeScript + Vite
Styling: Tailwind or CSS variables over design tokens
State: Zustand or Redux Toolkit, decided by prototype
Backend: Rust crates
Transport: Tauri invoke + event stream
Testing: Vitest + Playwright + Rust tests
```

## Why this helps Claude/design work

React/TypeScript makes the UI easier to evolve with Claude because:

- Components are smaller and easier to review.
- Design tokens can be centralized.
- Dialogs/docks can be previewed with mock data.
- Claude can safely modify layout without touching map logic.
- TypeScript DTOs catch frontend/backend drift.
- UI tests can run without real OTBM/client files.

## Ownership boundaries

Rust owns:

- map state
- item/tile mutation
- command stack
- undo/redo
- OTBM and XML I/O
- asset loading
- id mapping
- brush/autoborder behavior
- render frame planning
- diagnostics

React owns:

- menus
- docks
- dialogs
- panels
- visual theme
- filters/search text
- transient hover/selection UI
- command invocation wrappers

React must never become the source of truth for map data.

## Tauri command model

Backend commands should look like this:

```rust
#[derive(serde::Deserialize)]
pub struct SetTileGroundRequest {
    pub x: u16,
    pub y: u16,
    pub z: u8,
    pub item_id: u16,
}

#[derive(serde::Serialize)]
pub struct CommandResult {
    pub ok: bool,
    pub generation: u64,
    pub dirty: bool,
    pub diagnostics: Vec<String>,
}

#[tauri::command]
pub async fn set_tile_ground(
    request: SetTileGroundRequest,
    state: tauri::State<'_, EditorRuntime>,
) -> Result<CommandResult, String> {
    state.dispatch(EditorCommand::SetTileGround {
        x: request.x,
        y: request.y,
        z: request.z,
        item_id: request.item_id,
    })
}
```

Frontend wrapper:

```ts
import { invoke } from '@tauri-apps/api/core';

export type SetTileGroundRequest = {
  x: number;
  y: number;
  z: number;
  itemId: number;
};

export type CommandResult = {
  ok: boolean;
  generation: number;
  dirty: boolean;
  diagnostics: string[];
};

export function setTileGround(request: SetTileGroundRequest) {
  return invoke<CommandResult>('set_tile_ground', { request });
}
```

## Frontend package structure

```text
apps/desktop-tauri/
├── src-tauri/
└── src/
    ├── app/
    │   ├── AppShell.tsx
    │   ├── DockLayout.tsx
    │   └── menus/
    ├── editor/
    │   ├── canvas/
    │   ├── palette/
    │   ├── tool-options/
    │   ├── search/
    │   ├── map-properties/
    │   └── dialogs/
    ├── bindings/
    │   ├── commands.ts
    │   ├── events.ts
    │   └── dto.ts
    ├── state/
    │   ├── editorStore.ts
    │   ├── assetStore.ts
    │   └── uiStore.ts
    ├── design/
    │   ├── tokens.ts
    │   ├── theme.css
    │   └── components/
    └── testing/
        ├── fakeBackend.ts
        └── fixtures.ts
```

## First prototype slice

Do not start with full canvas parity. Start with a narrow vertical slice:

```text
Tauri shell boots
  -> Rust build_info command works
  -> React DiagnosticPanel renders result
  -> fake backend test passes
  -> real Tauri invoke smoke passes
```

Second slice:

```text
Rust brush catalog projection
  -> React PaletteDock renders entries
  -> select brush updates frontend state
  -> SetTileGround command dispatches to Rust
  -> backend emits tiles-changed event
  -> DiagnosticPanel records event
```

## Event protocol

```ts
export type BackendEvent =
  | { type: 'map-loaded'; path: string; generation: number; stats: MapStats }
  | { type: 'map-dirty-changed'; dirty: boolean; generation: number }
  | { type: 'tiles-changed'; generation: number; bounds: Rect3D[] }
  | { type: 'assets-reloaded'; status: AssetRuntimeStatus; warnings: string[] }
  | { type: 'render-snapshot-ready'; generation: number; frameId: number }
  | { type: 'diagnostic'; level: 'info' | 'warning' | 'error'; message: string };
```

## Component contract

Each complex component should include:

```text
ComponentName.tsx
ComponentName.test.tsx
ComponentName.fixture.ts
ComponentName.preview.tsx
```

Each component should have a short contract comment:

```ts
/**
 * PaletteDock renders brush families and brush entries.
 * It does not mutate map state directly.
 * It receives projections and dispatches command intents only.
 */
```

## UI migration order

1. App shell and diagnostics.
2. Preferences/client root UI.
3. Palette and tool options.
4. Search/find/replace dialogs.
5. Map statistics/properties.
6. House/town/spawn managers.
7. Canvas projection and events.
8. Full render surface.
9. Packaging/release.

## Non-goals for first React milestone

- Full OTBM editing parity.
- Full renderer replacement.
- Moving map state into TypeScript.
- Removing existing C++/Wx surface immediately.
- Implementing modern Canary assets before the compatibility matrix is written.
