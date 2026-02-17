OPENGL RENDERING SPECIALIST - Daily Report
Date: 2024-10-24
Files Scanned: 25
Review Time: 15 minutes
Quick Summary
Found 2 HIGH issues, 1 MEDIUM issue, and 1 LOW issue.
The "No tile batching" critical issue mentioned in the example appears to be resolved, as `MapLayerDrawer` uses `SpriteBatch` which implements instanced rendering.
However, static geometry is uploaded every frame due to the dynamic nature of `SpriteBatch`. PBO upload is disabled, causing synchronous texture uploads on the main thread.

Issue Count
CRITICAL: 0
HIGH: 2
MEDIUM: 1
LOW: 1

Issues Found
HIGH: Static tile geometry uploaded every frame
Location: source/rendering/drawers/map_layer_drawer.cpp
Problem: `MapLayerDrawer` iterates over all visible map nodes every frame and submits sprites to `SpriteBatch`. `SpriteBatch` uploads this instance data to the GPU every frame using persistent mapping.
Impact: While persistent mapping is efficient, uploading ~64KB of data per frame for static geometry wastes PCIe bandwidth and CPU time for scene traversal.
Fix: Create static VBOs for map chunks (e.g., 32x32 tiles) and only update them when tiles change. Use indirect draw commands to render visible chunks.
Expected improvement: Significantly reduced CPU overhead for scene traversal and reduced PCIe bandwidth usage.

HIGH: PBO texture upload disabled
Location: source/rendering/core/texture_atlas.cpp
Problem: `USE_PBO_FOR_SPRITE_UPLOAD` is not defined, causing `TextureAtlas::addSprite` to fall back to synchronous `glTextureSubImage3D` on the main thread.
Impact: Stalls the main thread during texture uploads, potentially causing frame drops when loading new areas or sprites.
Fix: Investigate and fix the "random sprite corruption" issue mentioned in the comments, then enable PBO support.
Expected improvement: Smoother frame rates during texture streaming.

MEDIUM: Overdraw from transparent layers
Location: source/rendering/drawers/map_layer_drawer.cpp
Problem: Layers are drawn back-to-front (Painter's Algorithm) without occlusion culling. Lower layers are drawn even if completely occluded by higher layers (e.g., solid ground on floor 7 occluding floor 8).
Impact: Wastes GPU fill rate on invisible pixels.
Fix: Implement occlusion culling (e.g., using a depth buffer pre-pass or software occlusion) to skip drawing occluded layers/tiles.
Expected improvement: Reduced GPU fill rate usage, especially on complex maps with many layers.

LOW: Minimap instance data upload
Location: source/rendering/drawers/minimap_renderer.cpp
Problem: `MinimapRenderer` uploads instance data for all visible minimap tiles every frame using `glNamedBufferSubData`.
Impact: Minor PCIe bandwidth usage.
Fix: Cache instance data in a VBO and only update when the view changes significantly or the map is modified.
Expected improvement: Minor reduction in PCIe bandwidth.
