# Rendering Engine Review

## Overview
This document reviews the current state of the rendering engine in `source/rendering/` with a focus on Single Responsibility Principle (SRP), Data-Oriented Design (DOD), and decoupling for future multi-threading support.

## Critical Issues

### 1. Tight Coupling between Map and Rendering
*   **File**: `source/map/map_region.h`
*   **Issue**: The `Floor` class contains `std::unique_ptr<RenderList> cached_render_list` and `bool is_render_dirty`.
*   **Impact**: The core Map data structure is directly coupled to the rendering implementation. This prevents clean separation and makes multi-threading difficult as the Map logic must manage rendering state.
*   **Recommendation**: Move `cached_render_list` and `is_render_dirty` out of `Floor` into a separate `RenderChunkCache` or `ChunkRenderer` system within the `rendering` module.

### 2. "God Class" Anti-Pattern in `MapDrawer`
*   **File**: `source/rendering/map_drawer.cpp` / `.h`
*   **Issue**: `MapDrawer` manages:
    *   The main render loop.
    *   Initialization of *all* other drawers (`TileRenderer`, `SpriteDrawer`, `ItemDrawer`, etc.).
    *   FBO management and resizing.
    *   Post-processing effects.
    *   Tooltips and UI overlays.
*   **Impact**: High complexity, difficult to test, and a bottleneck for changes. It violates SRP.
*   **Recommendation**: Split responsibilities.
    *   Move FBO/Post-processing to a `PostProcessPipeline` or `FrameGraph`.
    *   Move Drawer initialization to a `RendererFactory` or dependency injection container.
    *   Keep `MapDrawer` focused on the high-level orchestration of the frame.

### 3. Inefficient Memory Management in FBO
*   **File**: `source/rendering/map_drawer.cpp` (`UpdateFBO`)
*   **Issue**: `scale_texture` and `scale_fbo` are reallocated whenever `fbo_width` or `fbo_height` changes. These dimensions are derived from `view.zoom`.
*   **Impact**: Smooth zooming causes reallocation every frame, leading to severe stuttering and potential GPU memory fragmentation/exhaustion (the "hang" reported).
*   **Recommendation**: Implement a pooling strategy or allocate buffers at screen resolution (or a fixed max size) and use `glViewport` to render into a sub-region, only resizing when the window itself resizes or zoom crosses a significant threshold (e.g. power of 2).

### 4. Logic/Data Mixing in `MapLayerDrawer`
*   **File**: `source/rendering/drawers/map_layer_drawer.cpp`
*   **Issue**: `Extract` method iterates over the Map nodes and directly populates `RenderList`. While this is a step towards decoupling (separation of Extract/Submit), it still relies on pointer chasing through the Map structure (`MapNode` -> `Floor` -> `Tile` -> `Item`).
*   **Impact**: Cache misses due to pointer indirection. Hard to parallelize effectively without a dedicated render data structure.
*   **Recommendation**: The `Extract` phase should produce a linear, flat buffer of commands or data that `Submit` can consume. The `RenderList` is close to this, but its ownership by `Floor` is the problem.

### 5. `TileRenderer` Complexity
*   **File**: `source/rendering/drawers/tiles/tile_renderer.cpp`
*   **Issue**: `TileRenderer` handles:
    *   Minimap coloring.
    *   Item drawing (delegated to `ItemDrawer`).
    *   House highlighting.
    *   Creature drawing.
    *   Pattern calculation.
*   **Impact**: It knows too much about everything.
*   **Recommendation**: Simplify `TileRenderer` to be a coordinator that dispatches to specialized systems (e.g., `HouseHighlighter`, `GroundRenderer`, `ObjectRenderer`).

## Data-Oriented Design Analysis

*   **Current State**: The engine is largely Object-Oriented. `Tile` is a class with a vector of `Item` pointers. `Item` is a polymorphic class.
*   **DOD Goal**: We want to process data in contiguous arrays.
*   **Migration Path**:
    1.  **Intermediate**: Use `RenderList` (vector of `SpriteInstance`) as the primary interface. Isolate the creation of `RenderList` from the main thread if possible (requires thread-safe Map access or a copy of map data).
    2.  **Ideal**: Store map data in a flat array (SoA) instead of a tree of pointers. (Out of scope for this task but essential context).

## Plan for Modernization

1.  **Fix FBO Stutter**: Immediate priority.
2.  **Isolate Render State**: Create `RenderChunkCache` to hold `RenderList`s, keyed by `MapNode` ID/Coordinate.
3.  **Refactor Map**: Remove `cached_render_list` from `Floor`.
4.  **Refactor Drawers**: Update `MapLayerDrawer` to use `RenderChunkCache`.

This refactoring prepares the codebase for multi-threading by allowing the `RenderChunkCache` to be updated by worker threads (future work) without modifying the main `Map` structure during rendering, or at least managing the locks/dirty states explicitly in the renderer.
