# Rendering Engine Architecture Audit
## Goal
Audit the rendering engine in `source/rendering/` against Single Responsibility Principle (SRP) and Data-Oriented Design (DOD) principles. Focus strictly on correctness of design, separation of concerns, and clean data ownership to create a foundation ready for multi-threading.

---

## Part 1: Overall Architectural Assessment

The current rendering architecture is tightly coupled, relies heavily on global singletons (`g_gui.gfx`, `PostProcessManager::Instance()`), and frequently mixes input/business logic with rendering dispatch. The system is fundamentally designed around pointer-chasing (Array of Structures / Objects) rather than contiguous data arrays (Structure of Arrays), causing both cache inefficiency and implicit state sharing that blocks thread safety.

### Key Blockers for Modernization (Threading Blockers)

1. **Global `g_gui.gfx` Access in Leaf Nodes**: Deep rendering leaves like `SpriteDrawer` and `TileRenderer` query global state (`g_gui.gfx.getAtlasManager()`, `g_gui.gfx.getCreatureSprite()`) instead of having data passed to them or resolving it earlier in the pipeline.
2. **Hidden State Mutation**: `MapLayerDrawer` mutates the map (`editor->map.createLeaf`) during the rendering loop if a leaf is missing. Rendering must be strictly read-only.
3. **Mixed Responsibilities (God Classes)**:
   - `MapCanvas`: Mixes wxWidgets input routing, NanoVG initialization, rendering orchestration, screenshot handling, and state machine tracking (`dragging`, `floor`, `zoom`).
   - `TileRenderer`: Handles rendering, but also deeply couples into UI concerns by building Tooltip data and handling complex business logic for items.
4. **Pointer Chasing (AoS over SoA)**: The rendering loop iterates over `Tile*`, chasing pointers to `Item*`, `Creature*`, etc., evaluating business logic (e.g. `isContainer()`, `getDoorID()`) per frame instead of iterating over a flat array of rendering commands (DrawList).
5. **Singleton Anti-Patterns**: `PostProcessManager` is a classic Meyer's singleton, making execution order unpredictable and preventing multiple independent rendering viewports (e.g., mini-map vs main map).

---

## Part 2: Per-File Analysis

### Core Files

#### `source/rendering/map_drawer.h` / `.cpp`
* **What it does**: The central orchestrator for drawing the map, grids, layers, lighting, and post-processing.
* **SRP/DOD Violations**: Owns too many `unique_ptr`s to individual drawer types. Manages GL framebuffer state, coordinates passes, and orchestrates UI (tooltips/indicators) rendering.
* **Dependencies**: Heavily coupled to `Editor`, `MapCanvas`, `g_gui`, and every individual drawer.
* **State Issues**: Mutates `DrawingOptions` internally (e.g., calculating highlight pulses). Owns FBOs but relies on `canvas` for dimensions.
* **Threading Blockers**: `g_gui.gfx.ensureAtlasManager()` is called inside `Draw()`. Cannot be run on a background thread while the UI thread is active.
* **Refactoring**: Extract FBO/Post-Process management to a separate `RenderPipeline` class. Move pulse/animation calculations to an update phase, passing read-only constants to `Draw()`.

#### `source/rendering/core/graphics.h` / `.cpp`
* **What it does**: Manages sprites, images, textures, and the atlas manager.
* **SRP/DOD Violations**: Acts as a God class for all graphic resources. Manages both system memory (`std::unique_ptr<Sprite>`) and GPU memory (`AtlasManager`).
* **Dependencies**: Coupled to file loaders (`GameSpriteLoader`), `RenderTimer`, and `TextureGarbageCollector`.
* **State Issues**: `resident_images` and `resident_game_sprites` are `std::vector<void*>` mutated ad-hoc. State is global (`g_gui.gfx`).
* **Threading Blockers**: Loading sprites, clearing memory, and garbage collection are not thread-safe. Global access means any thread touching it risks data races.
* **Refactoring**: Split into `SpriteDatabase` (read-only system memory) and `GpuResourceManager` (GPU allocations).

#### `source/rendering/core/texture_garbage_collector.h` / `.cpp`
* **What it does**: Cleans up unused textures and software sprites based on time and thresholds.
* **SRP/DOD Violations**: Modifies external vectors passed by reference. Reads from global `g_settings` deep inside the logic.
* **Dependencies**: Coupled to `GameSprite` and `Image` internals.
* **State Issues**: Internal `cleanup_list` is mutated dynamically.
* **Threading Blockers**: Uses `time(nullptr)` internally. Modifying shared vectors blocks concurrent rendering.
* **Refactoring**: Change to a DOD approach: `GarbageCollector` should return a list of IDs/handles to be evicted, and the system owning those handles performs the actual deletion in a safe sync point.

#### `source/rendering/core/sprite_batch.h`
* **What it does**: High-performance instanced renderer using ring buffers.
* **SRP/DOD Violations**: Clean DOD design (batches instance data).
* **Dependencies**: OpenGL state.
* **State Issues**: Clean RAII.
* **Threading Blockers**: None, assuming it is only called from the thread holding the GL context.
* **Refactoring**: Keep as-is. Good example of modern design.

### Drawers Subsystem

#### `source/rendering/drawers/map_layer_drawer.cpp`
* **What it does**: Iterates over spatial hash grid leaves and dispatches tile rendering.
* **SRP/DOD Violations**: Mutates map state! If `live_client` is true and a node is missing, it calls `editor->map.createLeaf`.
* **Dependencies**: `TileRenderer`, `GridDrawer`, `Editor`, `Map`.
* **State Issues**: Modifying the map during rendering is a massive violation.
* **Threading Blockers**: Concurrent map modification will crash.
* **Refactoring**: Rendering must be strictly `const`. If a node is missing, render a loading placeholder but emit an event/request to the logic thread to create the node.

#### `source/rendering/drawers/tiles/tile_renderer.cpp`
* **What it does**: The heaviest rendering leaf. Calculates colors, parses items on a tile, builds tooltips, and dispatches sprite drawing.
* **SRP/DOD Violations**: Massive mixed responsibilities. It handles business logic (`isContainer()`, `getDoorID()`), builds UI tooltip strings (`FillItemTooltipData`), and does rendering.
* **Dependencies**: `g_gui`, `g_items`, `TooltipDrawer`, `ItemDrawer`, `SpriteDrawer`.
* **State Issues**: Extracts data dynamically from pointers (`Item*`, `Tile*`) per frame.
* **Threading Blockers**: Accesses global `g_items` and `g_gui`. Pointer chasing through complex hierarchies.
* **Refactoring**: Split into two phases: 1) A pure logic extraction phase that generates a flat array of `RenderCommand` structs (SoA). 2) A pure rendering phase that just loops over `RenderCommand`s and pushes them to `SpriteBatch`. Move Tooltip generation out of the rendering loop entirely (generate tooltips on mouse hover, not per frame).

#### `source/rendering/drawers/entities/sprite_drawer.cpp`
* **What it does**: Pushes sprites to the `SpriteBatch`.
* **SRP/DOD Violations**: Queries `g_gui.gfx.getAtlasManager()` directly to draw white pixels/boxes.
* **Dependencies**: Global `g_gui`.
* **State Issues**: Implicitly depends on the main UI class for a rendering resource.
* **Threading Blockers**: Global access.
* **Refactoring**: Pass `AtlasManager&` as a parameter to the drawing functions, or provide it to the constructor. Remove all `#include "ui/gui.h"`.

#### `source/rendering/drawers/entities/creature_drawer.cpp`
* **What it does**: Resolves outfit layers, mounts, and addons, and pushes them to `SpriteBatch`.
* **SRP/DOD Violations**: Queries `g_gui.gfx.getCreatureSprite()` directly.
* **Dependencies**: Global `g_gui`.
* **Refactoring**: Same as `SpriteDrawer`. Pass the resolved `GameSprite*` or `AtlasRegion` directly, rather than having the drawer look it up globally.

### UI & Remaining Subsystems

#### `source/rendering/ui/map_display.cpp` (`MapCanvas`)
* **What it does**: The main wxWidgets GL canvas window.
* **SRP/DOD Violations**: Extreme God class. Bridges wxWidgets events, OpenGL context creation, rendering setup, garbage collection triggers, and input state machines.
* **Dependencies**: Everything.
* **State Issues**: Owns tons of cursor state, click state, and logical modes (`dragging`, `boundbox_selection`).
* **Threading Blockers**: Combines UI thread events directly with OpenGL rendering calls.
* **Refactoring**: Extract input state machine to `InputController`. Extract OpenGL initialization/teardown to a pure `RenderContext`. Make `MapCanvas` a dumb view that just forwards events to controllers and swaps buffers.

#### `source/rendering/postprocess/post_process_manager.h`
* **What it does**: Manages screen shaders.
* **SRP/DOD Violations**: Singleton (`static PostProcessManager& Instance()`).
* **Dependencies**: Global access point.
* **State Issues**: Hidden global state.
* **Threading Blockers**: Singleton access without synchronization.
* **Refactoring**: Remove singleton. Inject `PostProcessManager` into `MapDrawer` or whatever owns the render pipeline.

---

## Part 3: Prioritized Refactoring Plan

### Tier 1: Quick Wins (Low Risk, High Momentum)
*Sweep the small dirt first.*

1. **Remove Singleton in PostProcessManager**
   - **Action**: Remove `Instance()` from `PostProcessManager`. Instantiate it as a unique pointer in `MapDrawer` (or `Editor`) and pass it down where needed.
   - **Why**: Eliminates hidden global state.
2. **Remove Global `g_gui` Access in Leaf Drawers**
   - **Action**: In `SpriteDrawer`, `CreatureDrawer`, and `TileRenderer`, remove all uses of `g_gui.gfx`. Pass `GraphicManager&` or `AtlasManager&` via constructor or method arguments.
   - **Why**: Breaks deep coupling to the UI layer and makes rendering components testable and isolated.
3. **Decouple `TextureGarbageCollector` from Global Settings**
   - **Action**: Remove `g_settings` from `TextureGarbageCollector`. Pass the threshold and pulse limits as parameters to `GarbageCollect()`.
   - **Why**: Pure functions/classes are easier to reason about and don't rely on hidden singletons.
4. **Fix Map Mutation in `MapLayerDrawer`**
   - **Action**: Stop calling `editor->map.createLeaf` in `Draw()`. Renderers must be `const`. If a leaf doesn't exist, queue a request to the main thread to create it, but do not create it mid-render.
   - **Why**: Rendering must never mutate the domain model.

### Tier 2: Medium Refactors (Scoped Structural Changes)
*Fix ownership and separate mixed responsibilities.*

1. **Extract Tooltip Generation from `TileRenderer`**
   - **Action**: Remove `FillItemTooltipData` and all tooltip-building logic from `TileRenderer::DrawTile`. Move this to a separate system (e.g., `TooltipController`) that only runs for the tile the mouse is currently hovering over.
   - **Why**: We are currently building complex tooltip strings for *every item on every tile on the screen every frame*, even though only one tooltip can be seen at a time. This is a massive CPU waste and SRP violation.
2. **Split `MapCanvas` Input vs. Rendering**
   - **Action**: Move all `OnMouse...`, `OnKey...`, `cursor_x`, `dragging`, etc., out of `MapCanvas` and into an `EditorInputState` class. `MapCanvas` should only own the `wxGLContext` and `SwapBuffers()`.
   - **Why**: Prepares the ground for moving rendering to a separate thread, completely detached from wxWidgets events.
3. **Refactor `DrawingOptions` to be Immutable during Rendering**
   - **Action**: Prevent `MapDrawer::SetupVars()` from mutating `DrawingOptions` (like `highlight_pulse`). Calculate the frame's uniform state *before* rendering begins, make it `const`, and pass it down the tree.
   - **Why**: Eliminates race conditions if multiple threads read the options.

### Tier 3: Heavy Lifts (Data-Oriented Redesign)
*Restructure data flows for cache locality and threading.*

1. **Implement a Render Command DrawList (SoA)**
   - **Action**: Stop pointer-chasing through `Tile* -> Item* -> ground->getType()` during the `Draw` call. Create an extraction phase that iterates the map and outputs a flat array of primitive drawing commands: `struct RenderCommand { float x, y, w, h; AtlasRegion region; DrawColor color; }`. The rendering phase then just iterates `std::vector<RenderCommand>` and blasts it to the `SpriteBatch`.
   - **Why**: This completely decouples domain data (Map/Items) from the GPU pipeline. It ensures maximum cache locality, sets up Data-Oriented Design, and allows the extraction phase and render phase to be parallelized later.
2. **Split `GraphicManager` into System vs. GPU**
   - **Action**: Separate `GraphicManager` into `SpriteDatabase` (owns the parsed game data, thread-safe, read-only after load) and `TexturePipeline` (owns `AtlasManager`, runs strictly on the render thread).
   - **Why**: Currently, logic threads and UI threads both interact with the same God class, mixing CPU memory with GL handles.
