OPENGL RENDERING SPECIALIST - Daily Report
Date: 2024-12-15 15:00
Files Scanned: 25
Review Time: 15 minutes
Quick Summary
Found 1 HIGH issue (Static Data Upload) and 1 MEDIUM issue (Batching Split). Codebase uses SpriteBatch effectively for draw call reduction, but uploads all geometry every frame.
Issue Count
CRITICAL: 0
HIGH: 1
MEDIUM: 1
LOW: 1
Issues Found
HIGH: Static tile geometry uploaded every frame
Location: source/rendering/core/sprite_batch.cpp:180 (flush)
Problem: Using RingBuffer (glNamedBufferSubData/mapping) every frame to upload all map tile sprites (10k+ sprites), even when the map is static.
Impact: Wastes PCIe bandwidth and CPU time (copying memory) for unchanging geometry.
Fix: Implement Chunked Rendering with static VBOs for map layers. Only update chunks when tiles change.
Expected improvement: Eliminates ~1-5MB/frame data transfer.

MEDIUM: Split batching between SpriteBatch and PrimitiveRenderer
Location: source/rendering/drawers/entities/item_drawer.cpp:244 (DrawHookIndicator)
Problem: Hook indicators use PrimitiveRenderer (triangles) while items use SpriteBatch. This forces a flush of SpriteBatch before primitives can be drawn (or forces primitives to be drawn after all sprites), causing incorrect depth/occlusion for primitives relative to other sprites.
Impact: Visual depth artifacts (hooks always on top) and potentially extra draw calls if interleaved.
Fix: Extend SpriteBatch to support generic primitives (or add a 'triangle' mode using degenerate quads) or render hooks as sprites.
Expected improvement: Unified batching, correct depth sorting.

LOW: Tooltip CPU Overhead in Rendering Loop
Location: source/rendering/drawers/tiles/tile_renderer.cpp:180 (DrawTile)
Problem: FillItemTooltipData is called for every visible item when tooltips are enabled, performing string lookups and copies.
Impact: CPU bottleneck reduces max FPS even if GPU is fast.
Fix: Cache tooltip data or only generate for the tile under cursor (unless specific "show all tooltips" mode is active and optimized).
Expected improvement: Reduced CPU frame time.

Summary Stats
Most common issue: Static Data Upload
Cleanest file: source/rendering/utilities/light_drawer.cpp (Good use of instancing/FBO)
Needs attention: source/rendering/core/sprite_batch.cpp (Needs static VBO support)
Estimated total speedup: 1-2ms frame time + reduced PCIe traffic.
Integration Details
Estimated Runtime: 10 minutes per review
Expected Findings: 0-1 issues per run
Automation: Run daily
