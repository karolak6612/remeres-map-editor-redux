# OPENGL RENDERING SPECIALIST - Daily Report

Date: 2024-10-24
Files Scanned: 18 (Rendering module)
Review Time: 20 minutes

## Quick Summary
Found 1 critical issue in `MinimapDrawer` / `MinimapRenderer` causing massive texture bind thrashing and potential GPU stalls. `MapLayerDrawer` also exhibits suboptimal static data usage.

**Issue Count**
- CRITICAL: 1
- HIGH: 2
- MEDIUM: 1
- LOW: 0

## Issues Found

### CRITICAL: Minimap Texture Update Thrashing
- **Location**: `source/rendering/drawers/minimap_renderer.cpp` (`updateRegion`)
- **Problem**: The method iterates through visible map chunks and performs a PBO map/write/unmap/bind/upload/unbind/fence sequence *inside the loop* for every chunk.
- **Impact**:
    - **Stalls**: `PixelBufferObject::mapWrite` waits for the fence. With only 2 buffers, if the loop runs >2 times per frame (which it does for any view larger than 2x1 chunks), it forces a CPU-GPU sync (stall) effectively serializing the uploads.
    - **Overhead**: Hundreds of GL calls (bind/unbind) per frame.
- **Fix**: Batch all chunk updates into a single PBO map operation. Map once, copy all data, unmap once, then issue multiple `glTextureSubImage3D` calls with offsets.

### HIGH: Static Map Geometry Uploaded Every Frame
- **Location**: `source/rendering/drawers/map_layer_drawer.cpp`
- **Problem**: `MapLayerDrawer` iterates visible nodes and `TileRenderer` submits vertices to `SpriteBatch` every frame. `SpriteBatch` maps and uploads a dynamic vertex buffer every frame.
- **Impact**: High CPU overhead for iteration and high PCIe bandwidth for uploading unchanged geometry.
- **Fix**: Implement cached VBOs for map chunks (e.g. 16x16 tiles) and only rebuild them when the map changes.

### HIGH: Minimap Redundant Updates
- **Location**: `source/rendering/drawers/minimap_drawer.cpp`
- **Problem**: `MinimapDrawer` calls `renderer->updateRegion` every frame with the full visible bounds, causing the texture to be updated continuously even if the map or view hasn't changed.
- **Impact**: Wasted CPU/GPU cycles generating and uploading minimap data.
- **Fix**: Track camera position and map modification generation; only update `updateRegion` when necessary or incrementally.

### MEDIUM: Immediate Mode-style Batching
- **Location**: `source/rendering/drawers/entities/item_drawer.cpp`
- **Problem**: Extensive use of `glBlitSquare` and `glDrawBox` which push individual quads to `SpriteBatch`.
- **Impact**: While batched, this adds to the CPU cost of rebuilding buffers every frame.
- **Fix**: Less critical than the above, but moving to cached geometry where possible would help.

## Summary Stats
- **Most common issue**: Static data upload every frame.
- **Needs attention**: `MinimapRenderer.cpp` (Critical stall/thrashing).
- **Estimated speedup**: Fixing MinimapRenderer could eliminate substantial frame time spikes during minimap updates.
