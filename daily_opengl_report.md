OPENGL RENDERING SPECIALIST - Daily Report
Date: 2024-10-24
Files Scanned: 22
Review Time: 15 minutes

Quick Summary
Found 1 critical issue in MinimapRenderer.cpp (unbatched draw calls) and 1 high issue in PrimitiveRenderer usage.
Estimated improvement: Minimap rendering 5-10x faster; Grid/Marker rendering 2x faster (reduced PCIe traffic).

Issue Count
CRITICAL: 1
HIGH: 1
MEDIUM: 0
LOW: 1

Issues Found

CRITICAL: Unbatched draw calls in Minimap Loop
Location: source/rendering/drawers/minimap_renderer.cpp:270-295
Problem: Calling glDrawArrays inside a nested loop for every minimap chunk (tile), changing uniforms (uMVP, uLayer) per call.
Impact: Creates multiple draw calls (e.g., 16-64) per frame for minimap, preventing batching and increasing CPU overhead.
Fix: Use instanced rendering for minimap chunks. Pass 'layer' as an instance attribute or batch all chunks into a single VBO/IBO with a texture array index.
Expected improvement: Reduces N draw calls to 1 draw call per minimap render pass.

HIGH: Static geometry uploaded every frame
Location: source/rendering/core/primitive_renderer.cpp:103 (and GridDrawer usage)
Problem: PrimitiveRenderer::flush uploads vertex data via glNamedBufferSubData every frame. GridDrawer draws the static grid using this renderer every frame.
Impact: Wastes PCIe bandwidth re-uploading identical grid lines (and static markers) every frame (60Hz+).
Fix: Create a dedicated StaticPrimitiveRenderer or cache the Grid VBO. Only update when view changes or grid settings change.
Expected improvement: Eliminates unnecessary vertex upload for static UI elements.

LOW/INFO: Good usage of SpriteBatch
Location: source/rendering/drawers/tiles/tile_renderer.cpp
Status: TileRenderer correctly delegates to SpriteBatch, which uses glDrawElementsInstanced / Multi-Draw Indirect.
Impact: Main map rendering is efficient. No "Draw Call Explosion" detected in main tile loop.

Summary Stats
Most common issue: Unbatched Minimap Drawing
Cleanest file: source/rendering/drawers/tiles/tile_renderer.cpp (Rendering logic is well-batched)
Needs attention: MinimapRenderer.cpp
Estimated total speedup: 2-3ms frame time (on low-end hardware)

Integration Details
Estimated Runtime: 15 minutes
Expected Findings: 1-2 issues per run
Automation: Run daily
