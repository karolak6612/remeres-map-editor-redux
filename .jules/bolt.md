## 2025-02-18 - Rendering Bottleneck: Per-Tile Logic
Learning: `TileRenderer::DrawTile` executes per visible tile every frame. Tooltips are currently drawn for all items, which is intentional (labels), but this design choice inherently limits performance on large views due to string operations.
Action: Future optimizations for tooltips should focus on caching the generated strings or using a more efficient font renderer, rather than culling them, as culling contradicts the "label" behavior.

## 2025-02-18 - Tooltip Allocations
Learning: `TooltipDrawer::draw` was allocating `std::vector<FieldLine>` inside a loop for every tooltip, causing allocator churn. Also `TileRenderer` was deep-copying `TooltipData` (containing strings/vectors) every frame.
Action: Use persistent scratch buffers (member variables) for immediate-mode rendering loops to avoid allocation. Use `std::move` for passing heavy transient data like tooltips.

## 2025-02-18 - Tooltip String Allocations
Learning: `CreateItemTooltipData` was allocating thousands of `std::string` objects (from `std::string_view`) every frame for item names. By pooling `TooltipData` objects and clearing them (which keeps `std::string` capacity), we eliminate these allocations entirely after warmup.
Action: Prefer modifying pooled objects in-place (`FillItemTooltipData`) over returning new objects by value in hot paths.

## 2025-02-18 - Minimap Color Caching
Learning: `Tile::update` failed to reset `minimapColor` when no color was found, leading to stale colors if items were removed. Also `deepCopy` produced `INVALID_MINIMAP_COLOR` tiles, forcing `getMiniMapColor` to scan items every time.
Action: Always initialize/reset cached values in `update()` methods to ensure state validity. Use `minimapColor = 0` (or valid "empty" value) instead of `INVALID` to enable unconditional fast-path access.

## 2024-05-22 - Map Loop Bounds & Parallax
Learning: `MapLayerDrawer` was using loop bounds derived from the camera's floor (z=7) for all layers. Due to parallax offset `(GROUND_LAYER - map_z) * TileSize`, this caused iteration over off-screen areas for other layers.
Action: Always calculate loop bounds specifically for the current `map_z` layer, accounting for the layer-specific offset. This allows removing per-tile visibility checks inside the loop.

## 2024-05-22 - Zero-Copy Tooltip Data
Learning: `TooltipData` used `std::string` members to store names from `ItemType` (static) and `Map` (long-lived). This caused unnecessary copies in the hot path of `FillItemTooltipData` which runs for every item.
Action: Use `std::string_view` in transient rendering structures when data source lifetime is guaranteed to outlive the frame (e.g. static assets).
