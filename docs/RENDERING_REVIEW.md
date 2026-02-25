# Rendering Engine Review & Modernization Plan

## 1. Executive Summary

The current rendering engine of Remere's Map Editor (RME) follows a **legacy Object-Oriented, Immediate-Mode** architectural pattern. While it uses modern OpenGL (via `SpriteBatch` and `PrimitiveRenderer`), the higher-level logic is heavily coupled, stateful, and single-threaded.

**Key Issues:**
*   **"God" Classes:** `MapDrawer` and `TileRenderer` orchestrate everything from game logic to UI tooltips and OpenGL calls.
*   **Violation of Single Responsibility Principle (SRP):** Rendering classes act as game logic processors (e.g., calculating container weights, checking door states).
*   **Pointer Chasing:** The engine traverses a graph of objects (`Map` -> `Node` -> `Tile` -> `Item*` -> `ItemType`) every frame, causing massive cache misses.
*   **Thread Unsafe:** Logic depends on global state (`g_items`, `g_gui`) and modifies editor state (Live Client node requests) during the render pass.

**Goal:** Move towards a **Data-Oriented, Retained-Mode (or Stateless Immediate)** design that separates **Data Preparation** from **Command Submission**, enabling safe multi-threading.

---

## 2. File-by-File Analysis

### 2.1. `source/rendering/map_drawer.cpp` (The "God" Class)
*   **Role:** Orchestrates the entire rendering frame.
*   **Issues:**
    *   **Coupling:** Owns instances of every single sub-drawer (`TileRenderer`, `ItemDrawer`, `CreatureDrawer`, `GridDrawer`, etc.).
    *   **State Management:** Manages FBOs, post-processing, and light buffers directly.
    *   **Logic Leak:** Handles "Live Client" logic directly in the render loop.
*   **Verdict:** Needs to be broken down. The `MapDrawer` should only be a high-level facade that delegates to a `RenderPipeline`.

### 2.2. `source/rendering/drawers/tiles/tile_renderer.cpp` (The Logic Bottleneck)
*   **Role:** Decides how to draw a single tile and its contents.
*   **Issues:**
    *   **SRP Violation:** It contains game logic! (e.g., `FillItemTooltipData` calculates container volumes and inspects item attributes).
    *   **Dependency Hell:** Constructor takes 8 pointers to other systems.
    *   **Performance:** It performs a linear scan of items on every tile every frame, checking conditions (is it a door? is it a container?) that rarely change.
*   **Verdict:** This is the biggest blocker for multi-threading. The "what to draw" logic must be separated from "how to draw".

### 2.3. `source/rendering/drawers/entities/item_drawer.cpp`
*   **Role:** Draws individual items.
*   **Issues:**
    *   **Mixed Responsibilities:** Handles coloring, pattern calculation (`PatternCalculator`), and also UI indicators (Hook/Door).
    *   **Coupling:** Calls `CreatureDrawer` (why does an item drawer need to draw creatures?).
*   **Verdict:** Should be a stateless function: `drawItem(Batch&, ItemData)`.

### 2.4. `source/rendering/core/sprite_batch.cpp` (The Good Part)
*   **Role:** Batches quads and submits to OpenGL.
*   **Status:** **Good.** This is a modern, efficient implementation using DSA (Direct State Access) and Ring Buffers.
*   **Action:** Keep as-is, but ensure it's only accessed from the main thread (or a dedicated render thread).

### 2.5. `source/rendering/drawers/map_layer_drawer.cpp`
*   **Role:** Iterates over the map grid.
*   **Issues:**
    *   **Row-Major Traversal:** It iterates `nd_start_x` then `nd_start_y`. Memory layout of `MapNode` children should be verified to ensure this matches the cache lines.
    *   **Logic Injection:** Checks `if (live && !nd->isVisible...)` and triggers network requests. This side-effect prevents this code from being thread-safe or pure.

---

## 3. Data-Oriented Design (DOD) Analysis

The current engine suffers from "Pointer Chasing". To render a frame, the CPU must fetch:
1.  `MapNode` (Heap)
2.  `Tile` (Heap/Pool)
3.  `Item` (Vector of Pointers -> Heap)
4.  `ItemType` (Global Array -> Random Access)
5.  `Sprite` (Pointer in ItemType -> Heap)

**Optimization Opportunity: The Frame Packet**
Instead of traversing this graph during render, we should generate a **Frame Packet** (or Render Queue).

**Current Flow (Single Threaded):**
```
Map -> Node -> Tile -> Logic(Item) -> GL Command
```

**Target Flow (Multi-Threaded):**
1.  **Extract (Parallel):** Map -> `FrameData` (SoA - Structure of Arrays).
    *   `FrameData` contains flat arrays: `std::vector<RenderCmd> opaque_commands;`
2.  **Submit (Main Thread):** `FrameData` -> `SpriteBatch`.

This decouples the *state of the map* from the *act of rendering*.

---

## 4. Multi-Threading Roadblocks

1.  **Global State:** Access to `g_items`, `g_gui`, and `editor->map` is unchecked.
2.  **OpenGL Context:** `SpriteBatch::draw` writes to a mapped buffer. While `SpriteBatch` handles the buffer, the act of mapping/unmapping is currently done inline.
3.  **Side Effects:** The renderer modifies the map (requesting nodes) and UI state (tooltips) during the draw traversal.

---

## 5. Modernization Roadmap

We recommend a 3-phase refactoring plan:

### Phase 1: SRP & Logic Extraction (Safe Refactoring)
*   **Objective:** Remove game logic from Renderers.
*   **Tasks:**
    1.  Move `FillItemTooltipData` out of `TileRenderer` into a `TooltipSystem`.
    2.  Move `LiveClient` node requests out of `MapLayerDrawer` into a `MapUpdateSystem` (pre-render update).
    3.  Refactor `ItemDrawer` to take a `RenderItem` struct (POD) instead of raw `Item*` + `Tile*` + `Options`.

### Phase 2: Stateless Drawers
*   **Objective:** Make drawers pure functions.
*   **Tasks:**
    1.  Convert `TileRenderer::DrawTile` into a static-like helper that writes to a `RenderList`.
    2.  Remove `Editor*` and `MapCanvas*` pointers from low-level drawers. Pass only necessary data (Zoom, Time, etc.).

### Phase 3: The Render Queue (Data-Oriented)
*   **Objective:** Decouple Logic from GL.
*   **Tasks:**
    1.  Introduce `RenderQueue`.
    2.  Split `MapDrawer::Draw` into `Extract()` (build Queue) and `Submit()` (flush Queue).
    3.  Parallelize `Extract()` using `std::for_each(std::execution::par, ...)` on map chunks.

## 6. Conclusion

The codebase is functional and uses modern GL features at the low level, but the high-level architecture is stuck in the past. By enforcing SRP and separating Data Extraction from Command Submission, we can achieve significant performance gains and thread safety without rewriting the low-level OpenGL code.
