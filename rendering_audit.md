# Rendering Engine Architecture Audit

## Overview
This document provides a thorough, file-by-file audit of the `source/rendering/` directory, evaluating the rendering engine against the Single Responsibility Principle (SRP) and Data-Oriented Design (DOD) principles.

The primary goal of this audit is to identify blockers to modernization—specifically focusing on issues that prevent the system from being refactored into a thread-safe, multi-threaded architecture. Key areas of concern include:
- Tight coupling and God classes
- Mixed responsibilities and shared mutable state
- Implicit state or hidden state buried inside classes
- State owned by the wrong abstraction
- State read/mutated from multiple places without clear ownership
- Global or singleton state causing unpredictable execution order

## File-by-File Audit

### `source/rendering//`

#### `source/rendering/map_drawer.cpp`
- **What it currently does:** Provides utilities or definitions. Executes drawing operations for map.
- **SRP/DOD Violations:** Massive SRP violation: orchestrates OpenGL state, manages all sub-drawers, manages UI tooltips, post-processing, and maintains CanvasState.
- **Dependencies/Coupling:** Ties together the MapCanvas (View), Editor (Controller/Model), and every single rendering sub-component.
- **State Ownership:** Owns over 25 std::unique_ptr instances for sub-drawers and maintains the entire CanvasState (clicks, dragging).
- **Thread-Safety Blockers:** Completely blocks multi-threading. Controls execution order sequentially while holding all state and binding to wxWidgets.
- **Concrete Refactoring Recommendations:** Dismantle MapDrawer into three distinct systems: `RendererCore`, `SceneBuilder`, and `UICanvas`. Remove `CanvasState` from the renderer completely.

#### `source/rendering/map_drawer.h`
- **What it currently does:** Defines or implements `TooltipRenderer, TileRenderer, ItemDrawer, Creature, NVGImageCache, HookIndicatorDrawer, TooltipCollector, Outfit, MapDrawer, MarkerDrawer, LiveCursorDrawer, Editor, MapCanvas, ShadeDrawer, CreatureDrawer, PreviewDrawer, DrawContext, BrushOverlayDrawer, BrushCursorDrawer, NVGcontext, PostProcessPipeline, SpriteDrawer, DoorIndicatorDrawer, FloorViewParams, GridDrawer, GameSprite, DragShadowDrawer, CreatureNameDrawer, MapLayerDrawer, LightRenderer, FloorDrawer`. Executes drawing operations for map.
- **SRP/DOD Violations:** Massive SRP violation: orchestrates OpenGL state, manages all sub-drawers, manages UI tooltips, post-processing, and maintains CanvasState.
- **Dependencies/Coupling:** Ties together the MapCanvas (View), Editor (Controller/Model), and every single rendering sub-component.
- **State Ownership:** Owns over 25 std::unique_ptr instances for sub-drawers and maintains the entire CanvasState (clicks, dragging).
- **Thread-Safety Blockers:** Completely blocks multi-threading. Controls execution order sequentially while holding all state and binding to wxWidgets.
- **Concrete Refactoring Recommendations:** Dismantle MapDrawer into three distinct systems: `RendererCore`, `SceneBuilder`, and `UICanvas`. Remove `CanvasState` from the renderer completely.

### `source/rendering/core/`

#### `source/rendering/core/animator.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/core/animator.h`
- **What it currently does:** Defines or implements `FrameDuration, Animator`.
- **SRP/DOD Violations:** Uses vectors of pointers or pointer-heavy structures, reducing CPU cache coherency (DOD violation). Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/core/atlas_lifecycle.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Relies on standard or internal utilities.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Statically appears thread-safe, assuming passed references are read-only.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/atlas_lifecycle.h`
- **What it currently does:** Defines or implements `AtlasLifecycle`.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Relies on standard or internal utilities.
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Statically appears thread-safe, assuming passed references are read-only.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/atlas_manager.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** Uses vectors of pointers or pointer-heavy structures, reducing CPU cache coherency (DOD violation).
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Maintains internal dynamic collections that are mutated at runtime.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/atlas_manager.h`
- **What it currently does:** Defines or implements `AtlasManager`.
- **SRP/DOD Violations:** Uses vectors of pointers or pointer-heavy structures, reducing CPU cache coherency (DOD violation). Acts as a centralized manager, potentially violating SRP by handling both resource lifecycle and access.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Maintains internal dynamic collections that are mutated at runtime.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/canvas_state.h`
- **What it currently does:** Defines or implements `BaseMap, CanvasState`.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Relies on standard or internal utilities.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Statically appears thread-safe, assuming passed references are read-only.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/coordinate_mapper.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/core/coordinate_mapper.h`
- **What it currently does:** Defines or implements `CoordinateMapper`.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`.
- **State Ownership:** Contains mutable `static` state or singleton patterns.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/core/draw_context.h`
- **What it currently does:** Defines or implements `ViewState, PrimitiveRenderer, SpriteBatch, DrawContext, DrawingOptions, BrushCursorDrawer, CanvasState, LightBuffer`.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to `DrawContext`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Statically appears thread-safe, assuming passed references are read-only.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/drawing_options.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/drawing_options.h`
- **What it currently does:** Defines or implements `RenderSettings, Settings, DrawingOptions, FrameState`.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Querying mutable `Editor` state mid-execution risks severe race conditions. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/core/editor_sprite.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `wxWidgets`.
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/core/editor_sprite.h`
- **What it currently does:** Defines or implements `EditorSprite`.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `wxWidgets`.
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/core/floor_view_params.h`
- **What it currently does:** Defines or implements `FloorViewParams`.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Relies on standard or internal utilities.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Statically appears thread-safe, assuming passed references are read-only.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/game_sprite.h`
- **What it currently does:** Defines or implements `CreatureSprite, SpritePreloader, GraphicManager, Sprite`.
- **SRP/DOD Violations:** Acts as a centralized manager, potentially violating SRP by handling both resource lifecycle and access. Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `wxWidgets`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/core/gl_resources.h`
- **What it currently does:** Defines or implements `GLProgram, GLBuffer, GLVertexArray, GLTextureResource, GLShader, GLFramebuffer`.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/gl_scoped_state.h`
- **What it currently does:** Defines or implements `ScopedGLCapability, ScopedGLFramebuffer, ScopedGLBlend, ScopedGLViewport`.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/gl_viewport.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/gl_viewport.h`
- **What it currently does:** Defines or implements `ViewState`.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/image.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/image.h`
- **What it currently does:** Defines or implements `Image, ImageType, ImageHandle`.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/light_buffer.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Relies on standard or internal utilities.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Statically appears thread-safe, assuming passed references are read-only.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/light_buffer.h`
- **What it currently does:** Defines or implements `Light, LightBuffer`.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Relies on standard or internal utilities.
- **State Ownership:** Maintains internal dynamic collections that are mutated at runtime.
- **Thread-Safety Blockers:** Statically appears thread-safe, assuming passed references are read-only.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/light_fbo.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/light_fbo.h`
- **What it currently does:** Defines or implements `LightFBO`.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/light_shader.cpp`
- **What it currently does:** Defines or implements `Light`.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/light_shader.h`
- **What it currently does:** Defines or implements `LightShader, GPULight`.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/minimap_colors.h`
- **What it currently does:** Defines or implements `RGBQuad`.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`.
- **State Ownership:** Contains mutable `static` state or singleton patterns.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/core/multi_draw_indirect_renderer.cpp`
- **What it currently does:** Provides utilities or definitions. Executes drawing operations for multi.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Make this a stateless OpenGL executor that consumes pre-built command buffers.

#### `source/rendering/core/multi_draw_indirect_renderer.h`
- **What it currently does:** Defines or implements `MultiDrawIndirectRenderer, DrawElementsIndirectCommand`. Executes drawing operations for multi.
- **SRP/DOD Violations:** Uses vectors of pointers or pointer-heavy structures, reducing CPU cache coherency (DOD violation).
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`). Maintains internal dynamic collections that are mutated at runtime.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Make this a stateless OpenGL executor that consumes pre-built command buffers.

#### `source/rendering/core/normal_image.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/normal_image.h`
- **What it currently does:** Defines or implements `NormalImage`.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/outfit_colorizer.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/core/outfit_colorizer.h`
- **What it currently does:** Defines or implements `OutfitColorizer`.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`.
- **State Ownership:** Contains mutable `static` state or singleton patterns.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/core/outfit_colors.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/core/outfit_colors.h`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/core/pixel_buffer_object.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/pixel_buffer_object.h`
- **What it currently does:** Defines or implements `PixelBufferObject`.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/primitive_renderer.cpp`
- **What it currently does:** Provides utilities or definitions. Executes drawing operations for primitive.
- **SRP/DOD Violations:** Uses vectors of pointers or pointer-heavy structures, reducing CPU cache coherency (DOD violation).
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Make this a stateless OpenGL executor that consumes pre-built command buffers.

#### `source/rendering/core/primitive_renderer.h`
- **What it currently does:** Defines or implements `PrimitiveRenderer, Vertex`. Executes drawing operations for primitive.
- **SRP/DOD Violations:** Uses vectors of pointers or pointer-heavy structures, reducing CPU cache coherency (DOD violation).
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Make this a stateless OpenGL executor that consumes pre-built command buffers.

#### `source/rendering/core/render_timer.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `wxWidgets`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/core/render_timer.h`
- **What it currently does:** Defines or implements `RenderTimer`.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `wxWidgets`.
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/core/ring_buffer.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/ring_buffer.h`
- **What it currently does:** Defines or implements `GLBuffer, RingBuffer`.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/shader_program.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/shader_program.h`
- **What it currently does:** Defines or implements `ShaderProgram, GLProgram`.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/shared_geometry.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Contains mutable `static` state or singleton patterns.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/shared_geometry.h`
- **What it currently does:** Defines or implements `SharedGeometry, ContextGeometry`.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/sprite_atlas_cache.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/core/sprite_atlas_cache.h`
- **What it currently does:** Defines or implements `SpriteAtlasCache, Outfit, AtlasRegion`.
- **SRP/DOD Violations:** Uses vectors of pointers or pointer-heavy structures, reducing CPU cache coherency (DOD violation). Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`.
- **State Ownership:** Maintains internal dynamic collections that are mutated at runtime.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/core/sprite_batch.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** Uses vectors of pointers or pointer-heavy structures, reducing CPU cache coherency (DOD violation).
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Maintains internal dynamic collections that are mutated at runtime.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/sprite_batch.h`
- **What it currently does:** Defines or implements `SpriteBatch`.
- **SRP/DOD Violations:** Uses vectors of pointers or pointer-heavy structures, reducing CPU cache coherency (DOD violation).
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/sprite_database.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `wxWidgets`.
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/core/sprite_database.h`
- **What it currently does:** Defines or implements `SpriteDatabase`.
- **SRP/DOD Violations:** Uses vectors of pointers or pointer-heavy structures, reducing CPU cache coherency (DOD violation). Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `wxWidgets`.
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/core/sprite_decompression.cpp`
- **What it currently does:** Defines or implements `DecompressionContext`.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`.
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/core/sprite_decompression.h`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`.
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/core/sprite_instance.h`
- **What it currently does:** Defines or implements `SpriteInstance`.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Relies on standard or internal utilities.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Statically appears thread-safe, assuming passed references are read-only.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/sprite_light.h`
- **What it currently does:** Defines or implements `SpriteLight`.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/core/sprite_metadata.h`
- **What it currently does:** Defines or implements `SpriteMetadata`.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`.
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/core/sprite_preloader.cpp`
- **What it currently does:** Defines or implements `PendingTask`.
- **SRP/DOD Violations:** Uses vectors of pointers or pointer-heavy structures, reducing CPU cache coherency (DOD violation). Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `wxWidgets`, OpenGL (`gl*` calls).
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`). Maintains internal dynamic collections that are mutated at runtime.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/core/sprite_preloader.h`
- **What it currently does:** Defines or implements `SpritePreloader, Task, Result`.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, OpenGL (`gl*` calls).
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Querying mutable `Editor` state mid-execution risks severe race conditions. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/core/sync_handle.h`
- **What it currently does:** Defines or implements `SyncHandle`.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/template_image.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/template_image.h`
- **What it currently does:** Defines or implements `TemplateImage`.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Relies on standard or internal utilities.
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Statically appears thread-safe, assuming passed references are read-only.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/text_renderer.cpp`
- **What it currently does:** Provides utilities or definitions. Executes drawing operations for text.
- **SRP/DOD Violations:** Uses vectors of pointers or pointer-heavy structures, reducing CPU cache coherency (DOD violation).
- **Dependencies/Coupling:** Coupled to `wxWidgets`, OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. wxWidgets dependencies force execution onto the main UI thread. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Make this a stateless OpenGL executor that consumes pre-built command buffers.

#### `source/rendering/core/text_renderer.h`
- **What it currently does:** Defines or implements `NVGcontext, TextRenderer`. Executes drawing operations for text.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Make this a stateless OpenGL executor that consumes pre-built command buffers.

#### `source/rendering/core/texture_array.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/texture_array.h`
- **What it currently does:** Defines or implements `TextureArray, GLTextureResource`.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/texture_atlas.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/texture_atlas.h`
- **What it currently does:** Defines or implements `TextureAtlas, AtlasRegion, FreeSlot`.
- **SRP/DOD Violations:** Uses vectors of pointers or pointer-heavy structures, reducing CPU cache coherency (DOD violation).
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`). Maintains internal dynamic collections that are mutated at runtime.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/texture_gc.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/texture_gc.h`
- **What it currently does:** Defines or implements `GameSprite, Sprite, SpriteDatabase, TextureGC`.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Relies on standard or internal utilities.
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`). Maintains internal dynamic collections that are mutated at runtime.
- **Thread-Safety Blockers:** Statically appears thread-safe, assuming passed references are read-only.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/view_state.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Relies on standard or internal utilities.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Statically appears thread-safe, assuming passed references are read-only.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/core/view_state.h`
- **What it currently does:** Defines or implements `ViewState, ViewBounds`.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

### `source/rendering/drawers/`

#### `source/rendering/drawers/map_layer_drawer.cpp`
- **What it currently does:** Provides utilities or definitions. Executes drawing operations for map.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `DrawContext`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Flatten the hierarchical object traversal into a centralized array of DOD `RenderCommands`.

#### `source/rendering/drawers/map_layer_drawer.h`
- **What it currently does:** Defines or implements `PrimitiveRenderer, GridDrawer, SpriteBatch, TileRenderer, MapLayerDrawer, Editor`. Executes drawing operations for map.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `DrawContext`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Flatten the hierarchical object traversal into a centralized array of DOD `RenderCommands`.

#### `source/rendering/drawers/minimap_drawer.cpp`
- **What it currently does:** Provides utilities or definitions. Executes drawing operations for minimap.
- **SRP/DOD Violations:** Massive SRP violation: orchestrates OpenGL state, manages all sub-drawers, manages UI tooltips, post-processing, and maintains CanvasState.
- **Dependencies/Coupling:** Ties together the MapCanvas (View), Editor (Controller/Model), and every single rendering sub-component.
- **State Ownership:** Owns over 25 std::unique_ptr instances for sub-drawers and maintains the entire CanvasState (clicks, dragging).
- **Thread-Safety Blockers:** Completely blocks multi-threading. Controls execution order sequentially while holding all state and binding to wxWidgets.
- **Concrete Refactoring Recommendations:** Dismantle MapDrawer into three distinct systems: `RendererCore`, `SceneBuilder`, and `UICanvas`. Remove `CanvasState` from the renderer completely.

#### `source/rendering/drawers/minimap_drawer.h`
- **What it currently does:** Defines or implements `Editor, MapCanvas, PrimitiveRenderer, MinimapDrawer`. Executes drawing operations for minimap.
- **SRP/DOD Violations:** Massive SRP violation: orchestrates OpenGL state, manages all sub-drawers, manages UI tooltips, post-processing, and maintains CanvasState.
- **Dependencies/Coupling:** Ties together the MapCanvas (View), Editor (Controller/Model), and every single rendering sub-component.
- **State Ownership:** Owns over 25 std::unique_ptr instances for sub-drawers and maintains the entire CanvasState (clicks, dragging).
- **Thread-Safety Blockers:** Completely blocks multi-threading. Controls execution order sequentially while holding all state and binding to wxWidgets.
- **Concrete Refactoring Recommendations:** Dismantle MapDrawer into three distinct systems: `RendererCore`, `SceneBuilder`, and `UICanvas`. Remove `CanvasState` from the renderer completely.

#### `source/rendering/drawers/minimap_renderer.cpp`
- **What it currently does:** Defines or implements `TileUpdate`. Executes drawing operations for minimap.
- **SRP/DOD Violations:** Uses vectors of pointers or pointer-heavy structures, reducing CPU cache coherency (DOD violation).
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Make this a stateless OpenGL executor that consumes pre-built command buffers.

#### `source/rendering/drawers/minimap_renderer.h`
- **What it currently does:** Defines or implements `MinimapRenderer, Map, InstanceData`. Executes drawing operations for minimap.
- **SRP/DOD Violations:** Uses vectors of pointers or pointer-heavy structures, reducing CPU cache coherency (DOD violation).
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Make this a stateless OpenGL executor that consumes pre-built command buffers.

### `source/rendering/drawers/cursors/`

#### `source/rendering/drawers/cursors/brush_cursor_drawer.cpp`
- **What it currently does:** Provides utilities or definitions. Executes drawing operations for brush.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to `DrawContext`, OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Make this a stateless OpenGL executor that consumes pre-built command buffers.

#### `source/rendering/drawers/cursors/brush_cursor_drawer.h`
- **What it currently does:** Defines or implements `BrushCursorDrawer, Brush`. Executes drawing operations for brush.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to `DrawContext`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Statically appears thread-safe, assuming passed references are read-only.
- **Concrete Refactoring Recommendations:** Flatten the hierarchical object traversal into a centralized array of DOD `RenderCommands`.

#### `source/rendering/drawers/cursors/drag_shadow_drawer.cpp`
- **What it currently does:** Provides utilities or definitions. Executes drawing operations for drag.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `DrawContext`, OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Make this a stateless OpenGL executor that consumes pre-built command buffers.

#### `source/rendering/drawers/cursors/drag_shadow_drawer.h`
- **What it currently does:** Defines or implements `CreatureDrawer, DragShadowDrawer, ItemDrawer, SpriteDrawer, Editor`. Executes drawing operations for drag.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `DrawContext`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Flatten the hierarchical object traversal into a centralized array of DOD `RenderCommands`.

#### `source/rendering/drawers/cursors/live_cursor_drawer.cpp`
- **What it currently does:** Provides utilities or definitions. Executes drawing operations for live.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `DrawContext`, `wxWidgets`, OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Make this a stateless OpenGL executor that consumes pre-built command buffers.

#### `source/rendering/drawers/cursors/live_cursor_drawer.h`
- **What it currently does:** Defines or implements `LiveSocket, Editor, LiveCursorDrawer`. Executes drawing operations for live.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `DrawContext`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Flatten the hierarchical object traversal into a centralized array of DOD `RenderCommands`.

### `source/rendering/drawers/entities/`

#### `source/rendering/drawers/entities/creature_drawer.cpp`
- **What it currently does:** Provides utilities or definitions. Executes drawing operations for creature.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `DrawContext`, OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Make this a stateless OpenGL executor that consumes pre-built command buffers.

#### `source/rendering/drawers/entities/creature_drawer.h`
- **What it currently does:** Defines or implements `CreatureDrawer, SpriteDrawer, CreatureDrawOptions`. Executes drawing operations for creature.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `DrawContext`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Flatten the hierarchical object traversal into a centralized array of DOD `RenderCommands`.

#### `source/rendering/drawers/entities/creature_name_drawer.cpp`
- **What it currently does:** Provides utilities or definitions. Executes drawing operations for creature.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `DrawContext`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Flatten the hierarchical object traversal into a centralized array of DOD `RenderCommands`.

#### `source/rendering/drawers/entities/creature_name_drawer.h`
- **What it currently does:** Defines or implements `CreatureNameDrawer, CreatureLabel, NVGcontext, Creature`. Executes drawing operations for creature.
- **SRP/DOD Violations:** Uses vectors of pointers or pointer-heavy structures, reducing CPU cache coherency (DOD violation).
- **Dependencies/Coupling:** Coupled to `DrawContext`.
- **State Ownership:** Maintains internal dynamic collections that are mutated at runtime.
- **Thread-Safety Blockers:** Statically appears thread-safe, assuming passed references are read-only.
- **Concrete Refactoring Recommendations:** Flatten the hierarchical object traversal into a centralized array of DOD `RenderCommands`.

#### `source/rendering/drawers/entities/item_drawer.cpp`
- **What it currently does:** Provides utilities or definitions. Executes drawing operations for item.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `DrawContext`, OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Make this a stateless OpenGL executor that consumes pre-built command buffers.

#### `source/rendering/drawers/entities/item_drawer.h`
- **What it currently does:** Defines or implements `BlitItemParams, DoorIndicatorDrawer, CreatureDrawer, Item, ItemDrawer, ItemType, SpritePatterns, Tile, SpriteDrawer, HookIndicatorDrawer`. Executes drawing operations for item.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `DrawContext`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Flatten the hierarchical object traversal into a centralized array of DOD `RenderCommands`.

#### `source/rendering/drawers/entities/sprite_drawer.cpp`
- **What it currently does:** Provides utilities or definitions. Executes drawing operations for sprite.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to `DrawContext`, `wxWidgets`, OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Make this a stateless OpenGL executor that consumes pre-built command buffers.

#### `source/rendering/drawers/entities/sprite_drawer.h`
- **What it currently does:** Defines or implements `AtlasManager, SpriteDrawer, AtlasRegion`. Executes drawing operations for sprite.
- **SRP/DOD Violations:** Acts as a centralized manager, potentially violating SRP by handling both resource lifecycle and access. Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `DrawContext`, `wxWidgets`, OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Make this a stateless OpenGL executor that consumes pre-built command buffers.

### `source/rendering/drawers/overlays/`

#### `source/rendering/drawers/overlays/brush_overlay_drawer.cpp`
- **What it currently does:** Provides utilities or definitions. Executes drawing operations for brush.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state. Large file size suggests multiple responsibilities (God-class tendency).
- **Dependencies/Coupling:** Coupled to `Editor`, `DrawContext`, OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Make this a stateless OpenGL executor that consumes pre-built command buffers.

#### `source/rendering/drawers/overlays/brush_overlay_drawer.h`
- **What it currently does:** Defines or implements `Brush, MapDrawer, CreatureDrawer, ItemDrawer, BrushOverlayDrawer, AtlasManager, SpriteDrawer, Editor`. Executes drawing operations for brush.
- **SRP/DOD Violations:** Acts as a centralized manager, potentially violating SRP by handling both resource lifecycle and access. Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `DrawContext`, `wxWidgets`, OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Make this a stateless OpenGL executor that consumes pre-built command buffers.

#### `source/rendering/drawers/overlays/door_indicator_drawer.cpp`
- **What it currently does:** Provides utilities or definitions. Executes drawing operations for door.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to `DrawContext`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Statically appears thread-safe, assuming passed references are read-only.
- **Concrete Refactoring Recommendations:** Flatten the hierarchical object traversal into a centralized array of DOD `RenderCommands`.

#### `source/rendering/drawers/overlays/door_indicator_drawer.h`
- **What it currently does:** Defines or implements `DoorRequest, NVGcontext, DoorIndicatorDrawer`. Executes drawing operations for door.
- **SRP/DOD Violations:** Uses vectors of pointers or pointer-heavy structures, reducing CPU cache coherency (DOD violation).
- **Dependencies/Coupling:** Coupled to `DrawContext`.
- **State Ownership:** Maintains internal dynamic collections that are mutated at runtime.
- **Thread-Safety Blockers:** Statically appears thread-safe, assuming passed references are read-only.
- **Concrete Refactoring Recommendations:** Flatten the hierarchical object traversal into a centralized array of DOD `RenderCommands`.

#### `source/rendering/drawers/overlays/grid_drawer.cpp`
- **What it currently does:** Provides utilities or definitions. Executes drawing operations for grid.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to `DrawContext`, `wxWidgets`, OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. wxWidgets dependencies force execution onto the main UI thread. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Make this a stateless OpenGL executor that consumes pre-built command buffers.

#### `source/rendering/drawers/overlays/grid_drawer.h`
- **What it currently does:** Defines or implements `AtlasManager, ViewBounds, GridDrawer`. Executes drawing operations for grid.
- **SRP/DOD Violations:** Acts as a centralized manager, potentially violating SRP by handling both resource lifecycle and access.
- **Dependencies/Coupling:** Coupled to `DrawContext`, `wxWidgets`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Flatten the hierarchical object traversal into a centralized array of DOD `RenderCommands`.

#### `source/rendering/drawers/overlays/hook_indicator_drawer.cpp`
- **What it currently does:** Provides utilities or definitions. Executes drawing operations for hook.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to `DrawContext`, OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Make this a stateless OpenGL executor that consumes pre-built command buffers.

#### `source/rendering/drawers/overlays/hook_indicator_drawer.h`
- **What it currently does:** Defines or implements `NVGcontext, HookIndicatorDrawer, HookRequest`. Executes drawing operations for hook.
- **SRP/DOD Violations:** Uses vectors of pointers or pointer-heavy structures, reducing CPU cache coherency (DOD violation).
- **Dependencies/Coupling:** Coupled to `DrawContext`.
- **State Ownership:** Maintains internal dynamic collections that are mutated at runtime.
- **Thread-Safety Blockers:** Statically appears thread-safe, assuming passed references are read-only.
- **Concrete Refactoring Recommendations:** Flatten the hierarchical object traversal into a centralized array of DOD `RenderCommands`.

#### `source/rendering/drawers/overlays/marker_drawer.cpp`
- **What it currently does:** Provides utilities or definitions. Executes drawing operations for marker.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `DrawContext`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Flatten the hierarchical object traversal into a centralized array of DOD `RenderCommands`.

#### `source/rendering/drawers/overlays/marker_drawer.h`
- **What it currently does:** Defines or implements `SpriteBatch, MarkerDrawer, Waypoint, Tile, SpriteDrawer, Editor`. Executes drawing operations for marker.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `DrawContext`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Flatten the hierarchical object traversal into a centralized array of DOD `RenderCommands`.

#### `source/rendering/drawers/overlays/preview_drawer.cpp`
- **What it currently does:** Provides utilities or definitions. Executes drawing operations for preview.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `DrawContext`, OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Make this a stateless OpenGL executor that consumes pre-built command buffers.

#### `source/rendering/drawers/overlays/preview_drawer.h`
- **What it currently does:** Defines or implements `CreatureDrawer, PreviewDrawer, ItemDrawer, Tile, SpriteDrawer, Editor`. Executes drawing operations for preview.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `DrawContext`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Flatten the hierarchical object traversal into a centralized array of DOD `RenderCommands`.

### `source/rendering/drawers/tiles/`

#### `source/rendering/drawers/tiles/floor_drawer.cpp`
- **What it currently does:** Provides utilities or definitions. Executes drawing operations for floor.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `DrawContext`, OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Make this a stateless OpenGL executor that consumes pre-built command buffers.

#### `source/rendering/drawers/tiles/floor_drawer.h`
- **What it currently does:** Defines or implements `CreatureDrawer, ItemDrawer, SpriteDrawer, Editor, FloorDrawer`. Executes drawing operations for floor.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `DrawContext`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Flatten the hierarchical object traversal into a centralized array of DOD `RenderCommands`.

#### `source/rendering/drawers/tiles/shade_drawer.cpp`
- **What it currently does:** Provides utilities or definitions. Executes drawing operations for shade.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to `DrawContext`, OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Make this a stateless OpenGL executor that consumes pre-built command buffers.

#### `source/rendering/drawers/tiles/shade_drawer.h`
- **What it currently does:** Defines or implements `ShadeDrawer`. Executes drawing operations for shade.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to `DrawContext`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Statically appears thread-safe, assuming passed references are read-only.
- **Concrete Refactoring Recommendations:** Flatten the hierarchical object traversal into a centralized array of DOD `RenderCommands`.

#### `source/rendering/drawers/tiles/tile_color_calculator.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Relies on standard or internal utilities.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/drawers/tiles/tile_color_calculator.h`
- **What it currently does:** Defines or implements `TileColorCalculator, DrawingOptions, Tile`.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Relies on standard or internal utilities.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/drawers/tiles/tile_renderer.cpp`
- **What it currently does:** Provides utilities or definitions. Executes drawing operations for tile.
- **SRP/DOD Violations:** Severe DOD violation. Rendering a tile involves O(N) pointer chases (checking each item, querying types, querying the Editor for highlights). Destroys CPU cache.
- **Dependencies/Coupling:** Takes a massive `TileRenderContext` holding references to 7 different sub-drawers and the Editor.
- **State Ownership:** Acts as a pass-through orchestrator, but holds references to all other stateful drawers.
- **Thread-Safety Blockers:** Unsafe: queries the Editor and traverses the mutable Map hierarchy during the render loop.
- **Concrete Refactoring Recommendations:** Remove entirely. The `SceneBuilder` should flatten the map chunk into cache-friendly arrays. Replace `TileRenderer` with a tight loop over a `RenderCommand` array.

#### `source/rendering/drawers/tiles/tile_renderer.h`
- **What it currently does:** Defines or implements `TileLocation, TooltipCollector, CreatureDrawer, MarkerDrawer, TileRenderer, ItemDrawer, CreatureNameDrawer, TileRenderContext, SpriteDrawer, Editor, FloorDrawer`. Executes drawing operations for tile.
- **SRP/DOD Violations:** Severe DOD violation. Rendering a tile involves O(N) pointer chases (checking each item, querying types, querying the Editor for highlights). Destroys CPU cache.
- **Dependencies/Coupling:** Takes a massive `TileRenderContext` holding references to 7 different sub-drawers and the Editor.
- **State Ownership:** Acts as a pass-through orchestrator, but holds references to all other stateful drawers.
- **Thread-Safety Blockers:** Unsafe: queries the Editor and traverses the mutable Map hierarchy during the render loop.
- **Concrete Refactoring Recommendations:** Remove entirely. The `SceneBuilder` should flatten the map chunk into cache-friendly arrays. Replace `TileRenderer` with a tight loop over a `RenderCommand` array.

### `source/rendering/io/`

#### `source/rendering/io/editor_sprite_loader.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `wxWidgets`, OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/io/editor_sprite_loader.h`
- **What it currently does:** Defines or implements `SpriteDatabase, SpriteLoader, EditorSpriteLoader`.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`.
- **State Ownership:** Contains mutable `static` state or singleton patterns.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/io/game_sprite_loader.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** Uses vectors of pointers or pointer-heavy structures, reducing CPU cache coherency (DOD violation). Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `wxWidgets`.
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`). Maintains internal dynamic collections that are mutated at runtime.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/io/game_sprite_loader.h`
- **What it currently does:** Defines or implements `GameSprite, GameSpriteLoader, FileReadHandle, SpriteLoader, SpriteDatabase`.
- **SRP/DOD Violations:** Uses vectors of pointers or pointer-heavy structures, reducing CPU cache coherency (DOD violation). Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `wxWidgets`.
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/io/screen_capture.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** Uses vectors of pointers or pointer-heavy structures, reducing CPU cache coherency (DOD violation).
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Maintains internal dynamic collections that are mutated at runtime.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/io/screen_capture.h`
- **What it currently does:** Defines or implements `ScreenCapture`.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Relies on standard or internal utilities.
- **State Ownership:** Contains mutable `static` state or singleton patterns.
- **Thread-Safety Blockers:** Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/io/screenshot_saver.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `wxWidgets`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/io/screenshot_saver.h`
- **What it currently does:** Defines or implements `ScreenshotSaver`.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `wxWidgets`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/io/sprite_loader.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `wxWidgets`.
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/io/sprite_loader.h`
- **What it currently does:** Defines or implements `SpriteLoader, SpriteDatabase`.
- **SRP/DOD Violations:** Uses vectors of pointers or pointer-heavy structures, reducing CPU cache coherency (DOD violation). Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `wxWidgets`.
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

### `source/rendering/postprocess/`

#### `source/rendering/postprocess/post_process_manager.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** Uses vectors of pointers or pointer-heavy structures, reducing CPU cache coherency (DOD violation).
- **Dependencies/Coupling:** Relies on standard or internal utilities.
- **State Ownership:** Maintains internal dynamic collections that are mutated at runtime.
- **Thread-Safety Blockers:** Statically appears thread-safe, assuming passed references are read-only.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/postprocess/post_process_manager.h`
- **What it currently does:** Defines or implements `PostProcessEffect, ShaderProgram, PostProcessManager`.
- **SRP/DOD Violations:** Uses vectors of pointers or pointer-heavy structures, reducing CPU cache coherency (DOD violation). Acts as a centralized manager, potentially violating SRP by handling both resource lifecycle and access.
- **Dependencies/Coupling:** Relies on standard or internal utilities.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/postprocess/post_process_pipeline.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to `wxWidgets`, OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. wxWidgets dependencies force execution onto the main UI thread. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/postprocess/post_process_pipeline.h`
- **What it currently does:** Defines or implements `PostProcessPipeline, PostProcessManager`.
- **SRP/DOD Violations:** Acts as a centralized manager, potentially violating SRP by handling both resource lifecycle and access.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

### `source/rendering/postprocess/effects/`

#### `source/rendering/postprocess/effects/effects.h`
- **What it currently does:** Defines or implements `PostProcessManager`.
- **SRP/DOD Violations:** Acts as a centralized manager, potentially violating SRP by handling both resource lifecycle and access.
- **Dependencies/Coupling:** Relies on standard or internal utilities.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Statically appears thread-safe, assuming passed references are read-only.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/postprocess/effects/scanline.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/postprocess/effects/screen.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Relies on standard or internal utilities.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Statically appears thread-safe, assuming passed references are read-only.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/postprocess/effects/xbrz.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

### `source/rendering/ui/`

#### `source/rendering/ui/brush_selector.cpp`
- **What it currently does:** Provides utilities or definitions. Bridges UI input with rendering.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/ui/brush_selector.h`
- **What it currently does:** Defines or implements `Editor, Tile, BrushSelector, Selection`. Bridges UI input with rendering.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `MapCanvas`.
- **State Ownership:** Contains mutable `static` state or singleton patterns.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Decouple from `MapCanvas`. Use command patterns for UI events.

#### `source/rendering/ui/clipboard_handler.cpp`
- **What it currently does:** Provides utilities or definitions. Bridges UI input with rendering.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `wxWidgets`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/ui/clipboard_handler.h`
- **What it currently does:** Defines or implements `Editor, ClipboardHandler, Selection`. Bridges UI input with rendering.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/ui/drawing_controller.cpp`
- **What it currently does:** Provides utilities or definitions. Bridges UI input with rendering.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `MapCanvas`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Decouple from `MapCanvas`. Use command patterns for UI events.

#### `source/rendering/ui/drawing_controller.h`
- **What it currently does:** Defines or implements `Editor, MapCanvas, DrawingController`. Bridges UI input with rendering.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `MapCanvas`, `wxWidgets`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Decouple from `MapCanvas`. Use command patterns for UI events.

#### `source/rendering/ui/keyboard_handler.cpp`
- **What it currently does:** Provides utilities or definitions. Bridges UI input with rendering.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `MapCanvas`, `wxWidgets`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Decouple from `MapCanvas`. Use command patterns for UI events.

#### `source/rendering/ui/keyboard_handler.h`
- **What it currently does:** Defines or implements `KeyboardHandler, MapCanvas`. Bridges UI input with rendering.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `MapCanvas`, `wxWidgets`.
- **State Ownership:** Contains mutable `static` state or singleton patterns.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Decouple from `MapCanvas`. Use command patterns for UI events.

#### `source/rendering/ui/map_display.cpp`
- **What it currently does:** Provides utilities or definitions. Bridges UI input with rendering.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state. Large file size suggests multiple responsibilities (God-class tendency).
- **Dependencies/Coupling:** Coupled to `Editor`, `MapCanvas`, `wxWidgets`, OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Decouple from `MapCanvas`. Use command patterns for UI events.

#### `source/rendering/ui/map_display.h`
- **What it currently does:** Defines or implements `MapWindow, SelectionController, MapDrawer, Item, ScreenshotController, DrawingOptions, DrawingController, MapMenuHandler, NVGcontext, Creature, AnimationTimer, MapCanvas`. Bridges UI input with rendering.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `MapCanvas`, `wxWidgets`, OpenGL (`gl*` calls).
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Decouple from `MapCanvas`. Use command patterns for UI events.

#### `source/rendering/ui/map_menu_handler.cpp`
- **What it currently does:** Provides utilities or definitions. Bridges UI input with rendering.
- **SRP/DOD Violations:** Uses vectors of pointers or pointer-heavy structures, reducing CPU cache coherency (DOD violation). Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `MapCanvas`, `wxWidgets`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Decouple from `MapCanvas`. Use command patterns for UI events.

#### `source/rendering/ui/map_menu_handler.h`
- **What it currently does:** Defines or implements `MapMenuHandler, Editor, MapCanvas`. Bridges UI input with rendering.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `MapCanvas`, `wxWidgets`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Decouple from `MapCanvas`. Use command patterns for UI events.

#### `source/rendering/ui/map_status_updater.cpp`
- **What it currently does:** Provides utilities or definitions. Bridges UI input with rendering.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `wxWidgets`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/ui/map_status_updater.h`
- **What it currently does:** Defines or implements `MapStatusUpdater, Editor`. Bridges UI input with rendering.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `wxWidgets`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/ui/minimap_window.cpp`
- **What it currently does:** Provides utilities or definitions. Bridges UI input with rendering.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `MapCanvas`, `wxWidgets`, OpenGL (`gl*` calls).
- **State Ownership:** Contains mutable `static` state or singleton patterns.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Decouple from `MapCanvas`. Use command patterns for UI events.

#### `source/rendering/ui/minimap_window.h`
- **What it currently does:** Defines or implements `MinimapWindow, MinimapDrawer`. Bridges UI input with rendering.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `wxWidgets`, OpenGL (`gl*` calls).
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/ui/navigation_controller.cpp`
- **What it currently does:** Provides utilities or definitions. Bridges UI input with rendering.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `MapCanvas`, `wxWidgets`.
- **State Ownership:** Contains mutable `static` state or singleton patterns.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Decouple from `MapCanvas`. Use command patterns for UI events.

#### `source/rendering/ui/navigation_controller.h`
- **What it currently does:** Defines or implements `NavigationController, MapCanvas`. Bridges UI input with rendering.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `MapCanvas`, `wxWidgets`.
- **State Ownership:** Contains mutable `static` state or singleton patterns.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Decouple from `MapCanvas`. Use command patterns for UI events.

#### `source/rendering/ui/nvg_image_cache.cpp`
- **What it currently does:** Provides utilities or definitions. Bridges UI input with rendering.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`.
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/ui/nvg_image_cache.h`
- **What it currently does:** Defines or implements `NVGcontext, NVGImageCache`. Bridges UI input with rendering.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/ui/popup_action_handler.cpp`
- **What it currently does:** Provides utilities or definitions. Bridges UI input with rendering.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `wxWidgets`.
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/ui/popup_action_handler.h`
- **What it currently does:** Defines or implements `PopupActionHandler, Editor, Tile`. Bridges UI input with rendering.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `MapCanvas`.
- **State Ownership:** Contains mutable `static` state or singleton patterns.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Decouple from `MapCanvas`. Use command patterns for UI events.

#### `source/rendering/ui/screenshot_controller.cpp`
- **What it currently does:** Provides utilities or definitions. Bridges UI input with rendering.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `MapCanvas`, `wxWidgets`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Decouple from `MapCanvas`. Use command patterns for UI events.

#### `source/rendering/ui/screenshot_controller.h`
- **What it currently does:** Defines or implements `ScreenshotSaver, MapCanvas, ScreenshotController`. Bridges UI input with rendering.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `MapCanvas`, `wxWidgets`.
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Decouple from `MapCanvas`. Use command patterns for UI events.

#### `source/rendering/ui/selection_controller.cpp`
- **What it currently does:** Provides utilities or definitions. Bridges UI input with rendering.
- **SRP/DOD Violations:** Uses vectors of pointers or pointer-heavy structures, reducing CPU cache coherency (DOD violation). Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `MapCanvas`, `wxWidgets`.
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`). Maintains internal dynamic collections that are mutated at runtime.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Decouple from `MapCanvas`. Use command patterns for UI events.

#### `source/rendering/ui/selection_controller.h`
- **What it currently does:** Defines or implements `SelectionController, Tile, SelectionThread, Editor, MapCanvas`. Bridges UI input with rendering.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `MapCanvas`, `wxWidgets`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Decouple from `MapCanvas`. Use command patterns for UI events.

#### `source/rendering/ui/sprite_icon_renderer.cpp`
- **What it currently does:** Provides utilities or definitions. Executes drawing operations for sprite.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `wxWidgets`, OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Make this a stateless OpenGL executor that consumes pre-built command buffers.

#### `source/rendering/ui/sprite_icon_renderer.h`
- **What it currently does:** Defines or implements `SpriteIconRenderer, CachedDC, RenderKey, RenderKeyHash`. Executes drawing operations for sprite.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `wxWidgets`.
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/ui/sprite_size.h`
- **What it currently does:** Provides utilities or definitions. Bridges UI input with rendering.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/ui/tooltip_collector.cpp`
- **What it currently does:** Provides utilities or definitions. Bridges UI input with rendering.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/ui/tooltip_collector.h`
- **What it currently does:** Defines or implements `ContainerItem, TooltipCategory, TooltipCollector, TooltipData`. Bridges UI input with rendering.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`.
- **State Ownership:** Maintains internal dynamic collections that are mutated at runtime.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/ui/tooltip_data_extractor.cpp`
- **What it currently does:** Provides utilities or definitions. Bridges UI input with rendering.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Relies on standard or internal utilities.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/ui/tooltip_data_extractor.h`
- **What it currently does:** Defines or implements `ItemType, Position, Item, TooltipData`. Bridges UI input with rendering.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Relies on standard or internal utilities.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Statically appears thread-safe, assuming passed references are read-only.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/ui/tooltip_renderer.cpp`
- **What it currently does:** Provides utilities or definitions. Executes drawing operations for tooltip.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `DrawContext`, `wxWidgets`, OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Make this a stateless OpenGL executor that consumes pre-built command buffers.

#### `source/rendering/ui/tooltip_renderer.h`
- **What it currently does:** Defines or implements `TooltipRenderer, FieldLine, DrawContext, LayoutMetrics, NVGcontext, NVGImageCache`. Executes drawing operations for tooltip.
- **SRP/DOD Violations:** Uses vectors of pointers or pointer-heavy structures, reducing CPU cache coherency (DOD violation). Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `DrawContext`.
- **State Ownership:** Maintains internal dynamic collections that are mutated at runtime.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/ui/zoom_controller.cpp`
- **What it currently does:** Provides utilities or definitions. Bridges UI input with rendering.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `MapCanvas`, `wxWidgets`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Decouple from `MapCanvas`. Use command patterns for UI events.

#### `source/rendering/ui/zoom_controller.h`
- **What it currently does:** Defines or implements `ZoomController, MapCanvas`. Bridges UI input with rendering.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `MapCanvas`, `wxWidgets`.
- **State Ownership:** Contains mutable `static` state or singleton patterns.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`). Decouple from `MapCanvas`. Use command patterns for UI events.

### `source/rendering/utilities/`

#### `source/rendering/utilities/fps_counter.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `wxWidgets`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/utilities/fps_counter.h`
- **What it currently does:** Defines or implements `FPSCounter`.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `wxWidgets`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/utilities/frame_pacer.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/utilities/frame_pacer.h`
- **What it currently does:** Defines or implements `FramePacer`.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/utilities/icon_renderer.cpp`
- **What it currently does:** Provides utilities or definitions. Executes drawing operations for icon.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to `wxWidgets`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/utilities/icon_renderer.h`
- **What it currently does:** Defines or implements `NVGcontext, NVGcolor`. Executes drawing operations for icon.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Relies on standard or internal utilities.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Statically appears thread-safe, assuming passed references are read-only.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/utilities/light_calculator.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/utilities/light_calculator.h`
- **What it currently does:** Defines or implements `LightCalculator`.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/utilities/light_renderer.cpp`
- **What it currently does:** Provides utilities or definitions. Executes drawing operations for light.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to `DrawContext`, OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context.
- **Concrete Refactoring Recommendations:** Make this a stateless OpenGL executor that consumes pre-built command buffers.

#### `source/rendering/utilities/light_renderer.h`
- **What it currently does:** Defines or implements `LightShader, LightRenderer, LightFBO, GPULight`. Executes drawing operations for light.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to `DrawContext`.
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Statically appears thread-safe, assuming passed references are read-only.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/utilities/pattern_calculator.h`
- **What it currently does:** Defines or implements `PatternCalculator, SpritePatterns`.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to OpenGL (`gl*` calls).
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Direct OpenGL calls strictly bind execution to the main thread context. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/utilities/sprite_icon_generator.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `wxWidgets`.
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/utilities/sprite_icon_generator.h`
- **What it currently does:** Defines or implements `SpriteIconGenerator`.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `wxWidgets`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/utilities/tile_describer.cpp`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to `wxWidgets`.
- **State Ownership:** Appears mostly stateless or manages simple value types.
- **Thread-Safety Blockers:** wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

#### `source/rendering/utilities/tile_describer.h`
- **What it currently does:** Defines or implements `TileDescriber, Map, Tile`.
- **SRP/DOD Violations:** Deeply coupled with Editor/MapCanvas God-objects, violating SRP by mixing rendering execution with application state.
- **Dependencies/Coupling:** Coupled to `Editor`, `wxWidgets`.
- **State Ownership:** Contains mutable `static` state or singleton patterns.
- **Thread-Safety Blockers:** Querying mutable `Editor` state mid-execution risks severe race conditions. wxWidgets dependencies force execution onto the main UI thread. Static/global mutable state prevents concurrent execution.
- **Concrete Refactoring Recommendations:** Remove `Editor` dependency completely; pass a read-only data view (`RenderStateView`).

#### `source/rendering/utilities/wx_utils.h`
- **What it currently does:** Provides utilities or definitions.
- **SRP/DOD Violations:** No severe SRP/DOD violations detected statically. Appears focused.
- **Dependencies/Coupling:** Coupled to `wxWidgets`.
- **State Ownership:** Owns heap-allocated resources (`std::unique_ptr`).
- **Thread-Safety Blockers:** wxWidgets dependencies force execution onto the main UI thread.
- **Concrete Refactoring Recommendations:** Ensure strict const-correctness and pass-by-value/span where possible.

## Overall Architectural Assessment
The `source/rendering/` engine is currently organized as a **Procedural Object-Oriented monolith**, meaning it is built using objects (`ItemDrawer`, `FloorDrawer`, `MapDrawer`), but the execution flow is entirely procedural, driven by massive static function chains and deep dependency injection.

This architecture actively resists Data-Oriented Design (DOD) and blocks multi-threading for the following systemic reasons:

### 1. The "Editor" God-Object Dependency
Almost every component in the rendering pipeline (down to the lowest-level tile drawers) holds a reference to, or takes a pointer to, the `Editor` class or `MapCanvas` class.
- **The Problem:** The renderer is not a "dumb pipe" that takes data and draws it. It is actively querying the application's business logic, user interface state, and game state (e.g., checking if the user is dragging, looking at the current house ID, or querying the current brush).
- **The Blocker:** Because the renderer queries the `Editor` during the `draw()` loop, you cannot run the renderer on a separate thread. If the user clicks or changes a brush while the renderer thread is reading the `Editor` state, a race condition will occur.

### 2. State Owned by the Wrong Abstractions
Drawers like `DoorIndicatorDrawer`, `SpriteBatch`, and `MapDrawer` hold mutable state.
- **The Problem:** The `MapDrawer` holds the `CanvasState` (where the user clicked). The `SpriteBatch` holds the list of vertices being built.
- **The Blocker:** If state is buried inside the classes executing the logic, you cannot have multiple command buffers. A true DOD/Multi-threaded renderer separates the "Command Data" (a struct of vertices/commands) from the "Command Executor" (the OpenGL wrapper).

### 3. Immediate Mode Execution vs. Retained/Command-Buffer Mode
The current rendering flow traverses the map hierarchy (`MapDrawer` -> `MapLayerDrawer` -> `FloorDrawer` -> `TileRenderer` -> `ItemDrawer` -> `SpriteBatch`) and executes state-building *interleaved* with logic.
- **The Problem:** Rendering a tile involves O(N) pointer chases (checking each item on a tile, querying its type, querying its sprite, querying the `Editor` for highlights). This destroys the CPU instruction cache and data cache.
- **The Blocker:** To modernize, this must become a two-phase pipeline:
  1. **Update/Culling Phase:** Traverse the spatial grid, outputting flat arrays of integers (`[SpriteID, X, Y, Z, ColorMask]`).
  2. **Render Phase:** A tight, non-branching loop that uploads this array to the GPU (or processes it into vertices).

### Summary of Blockers
To achieve a thread-safe, multi-threaded rendering architecture, the engine must transition from:
**"Drawers querying the Editor"** ➡️ **"The Editor building Command Lists for the Renderer."**

## Prioritized Refactoring Plan

This plan focuses entirely on correctness of design, separation of concerns, and clean data ownership—not on parallelism. A clean, data-oriented design naturally reduces CPU bottlenecks and makes multi-threading a much smaller problem. Sweeping the small dirt first makes room to move the furniture.

### Tier 1: Quick Wins
*Low-risk, self-contained changes with minimal dependencies. Rename, extract, split, or isolate things that are clearly wrong and easy to fix without touching much else.*

1. **Decouple Overlays from Internal State:**
   - **Files:** `door_indicator_drawer.h`, `hook_indicator_drawer.h`
   - **Action:** Remove the `std::vector<DoorRequest>` and `addDoor()`/`clear()` methods from the drawer classes. Make the `draw()` method accept `std::span<const DoorRequest>`. Let the caller (who knows the map state) own the array.
2. **Remove UI State from Core Renderer Structs:**
   - **Files:** `canvas_state.h`, `drawing_options.h`, `draw_context.h`
   - **Action:** Remove `CanvasState` (which tracks "last click map x", "is dragging draw", "is pasting") from the rendering core. The UI system should own this state. The renderer should only be told "Draw a transparent rectangle from A to B" (a rendering primitive) instead of "The user is dragging" (a UI concept).
3. **Isolate `MapMenuHandler` Logic:**
   - **Files:** `ui/map_menu_handler.h`, `ui/popup_action_handler.h`, `ui/brush_selector.h`
   - **Action:** Stop passing `Editor&` into massive static function collections. Convert these into simple Command objects (e.g., `RotateItemCommand`) that the `Editor` executes, breaking the strict dependency between the wxWidgets UI rendering code and the Editor logic.

### Tier 2: Medium Refactors
*Changes that require touching a few systems but are well-scoped. Fixing ownership of state, breaking apart classes with mixed responsibilities, replacing implicit dependencies with explicit ones.*

1. **Remove `Editor&` from All Drawers:**
   - **Files:** `map_drawer.h`, `tiles/floor_drawer.h`, `tiles/tile_renderer.h`, `overlays/preview_drawer.h`, etc.
   - **Action:** The `Editor` class is a god-object. Every drawer takes an `Editor&` to query random state (selection, current brush, highlights). This makes the renderer impossible to test or isolate. Create a strict `RenderStateView` struct that contains *only* the data the renderer needs for the current frame (e.g., `std::vector<Position> selection_highlights; const Brush* active_brush;`). Pass this read-only struct down the tree.
2. **Flatten the Drawer Hierarchy (Dependency Injection Cleanup):**
   - **Files:** `drawers/tiles/floor_drawer.h`, `drawers/overlays/preview_drawer.h`
   - **Action:** Stop passing pointers to `ItemDrawer`, `SpriteDrawer`, and `CreatureDrawer` into `FloorDrawer::draw()`. A single orchestrator (a `SceneBuilder`) should handle the looping logic. The `FloorDrawer` should just draw floors.
3. **Refactor `SpriteIconRenderer` (UI Compositing):**
   - **Files:** `ui/sprite_icon_renderer.h`
   - **Action:** Remove `wxMemoryDC` and CPU-side blitting. Make the UI request an OpenGL texture handle from the `AtlasManager` and `SpriteBatch`. The UI should just draw a textured quad using hardware acceleration.

### Tier 3: Heavy Lifts
*Structural changes that require redesigning subsystems, untangling deeply coupled components, or fundamentally changing how data flows through the engine. These should only be approached after the quick wins and medium refactors are done.*

1. **Dismantle `MapDrawer` into Builder/Executor:**
   - **Files:** `map_drawer.h`, `map_drawer.cpp`
   - **Action:** Split the massive `MapDrawer` into three systems:
     - `SceneBuilder`: Takes the Map and the `RenderStateView` and traverses the visible map chunk. It outputs a flat `std::vector<RenderCommand>` (a DOD-friendly array of `[SpriteID, X, Y, Z, ColorMask]`).
     - `Renderer`: A completely stateless execution pipeline. It takes the `std::vector<RenderCommand>`, uploads it to a `SpriteBatch` or VBO, and issues OpenGL draw calls.
     - `UICanvas`: Handles wxWidgets resizing and clicks, passing input events up to the Editor.
   - *Why this matters:* This fully separates "What to draw" from "How to draw it". The `SceneBuilder` can eventually be multi-threaded because it just outputs arrays. The `Renderer` remains strictly on the main thread (OpenGL context).
2. **Redesign `TileRenderer` for Data-Oriented Design:**
   - **Files:** `drawers/tiles/tile_renderer.h`
   - **Action:** Instead of doing deep pointer chasing (`tile->getTopItems()`, `item->getSprite()`, `tooltip_collector->add()`) in the middle of the render loop, the `SceneBuilder` (from step 1) should flatten the entire visual representation of the map chunk into cache-friendly arrays. The `TileRenderer` disappears entirely, replaced by a simple loop over the `RenderCommand` array.
3. **Background Atlas Generation:**
   - **Files:** `core/atlas_manager.h`, `core/sprite_preloader.h`
   - **Action:** Decouple the logical bin-packing of the texture atlas from the OpenGL texture upload. Build the `TextureAtlas` (the CPU byte array) asynchronously on a background thread as sprites are loaded from disk. Once a chunk is ready, issue a single non-blocking `glTexSubImage2D` command on the main thread.
