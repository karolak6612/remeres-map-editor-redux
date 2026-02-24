OPENGL RENDERING SPECIALIST - Daily Report
Date: 2024-12-15 14:30
Files Scanned: 20
Review Time: 15 minutes
Quick Summary
Found 2 issues:
1. HIGH: FBO reallocation during zoom in MapDrawer.cpp.
2. MEDIUM: Static data upload every frame in SpriteBatch.cpp.
Batching is implemented correctly across TileRenderer and other drawers. No draw call explosion found.

Issue Count
CRITICAL: 0
HIGH: 1
MEDIUM: 1
LOW: 0

Issues Found
HIGH: FBO Reallocation on Zoom
Location: source/rendering/map_drawer.cpp:168
Problem: UpdateFBO reallocates texture and FBO on every frame when zoom changes, causing stutter.
Impact: Severe texture memory churn during smooth zoom operations.
Fix: Allocate FBO at screen resolution or use power-of-two sizing to minimize reallocations.
Expected improvement: Eliminates stutter during zoom.

MEDIUM: Static Data Upload Every Frame
Location: source/rendering/core/sprite_batch.cpp:248
Problem: Uploading instance data (x, y, uv, tint) for every sprite every frame to dynamic VBO (RingBuffer).
Impact: Wastes PCIe bandwidth (approx 320KB/frame for 5000 sprites). Minor impact on modern hardware but technically redundant for static tiles.
Fix: Implement static VBOs for map chunks and use camera uniforms for scrolling.
Expected improvement: Reduces PCIe traffic.

Summary Stats
Most common issue: Dynamic allocation (2 locations)
Cleanest file: TileRenderer.cpp (uses SpriteBatch correctly)
Needs attention: MapDrawer.cpp (FBO logic)
Estimated total speedup: Eliminates stutter during zoom.
