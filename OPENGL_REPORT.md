OPENGL RENDERING SPECIALIST - Daily Report
Date: 2024-05-22
Files Scanned: 15
Review Time: 15 minutes
Quick Summary
Found 1 HIGH priority issue related to static data streaming. The codebase generally uses efficient batching and instancing via `SpriteBatch`, avoiding the "draw call explosion" pattern. However, map geometry is re-uploaded every frame using persistent mapping, which consumes PCIe bandwidth unnecessarily for static content.

Issue Count
CRITICAL: 0
HIGH: 1
MEDIUM: 0
LOW: 1

Issues Found
HIGH: Static tile geometry uploaded every frame
Location: source/rendering/core/sprite_batch.cpp:234 (memcpy to mapped buffer)
Problem: The rendering pipeline recalculates screen coordinates on the CPU and uploads sprite instance data (position, UVs, color) to a RingBuffer every frame via persistent mapping. Even though the map tiles are static, their screen positions change with the camera scroll, necessitating this re-upload.
Impact: Wastes CPU cycles on coordinate calculation and PCIe bandwidth (approx 20MB/s for 10k sprites at 60fps) transferring data that could be static in VRAM.
Fix: Implement a chunk-based rendering system where map geometry (tile positions relative to chunk origin) is uploaded once to a static VBO. Use a uniform buffer for the View/Projection matrix to handle camera scrolling in the vertex shader. Only dynamic objects (creatures, animations) should use the streaming path.
Expected improvement: Significantly reduced CPU overhead per frame and lower PCIe bus usage.

LOW: Opportunity for compressed texture formats
Location: source/rendering/core/texture_atlas.cpp (inferred)
Problem: Textures appear to be loaded as uncompressed RGBA8.
Impact: Higher VRAM usage and texture fetch bandwidth.
Fix: Use block compression (BC3/DXT5) for static assets where appropriate.

Summary Stats
Most common issue: Static Data Upload (systemic)
Cleanest file: source/rendering/core/sprite_batch.cpp (Well-structured batching and instancing implementation)
Needs attention: source/rendering/drawers/tiles/tile_renderer.cpp (High CPU overhead per tile for coordinate calculation)

Integration Details
Estimated Runtime: N/A
Expected Findings: 1 High issue
Automation: Run daily to monitor for regressions in batching or new unoptimized draw paths.
