## 2024-10-26 - Optimization of Item Tooltips and Viewport Culling
Finding: Generating tooltip data for every item in the viewport involved excessive string allocation and copying, even for items with no tooltip info. Additionally, the render loop calculated screen coordinates twice for every visible tile.
Impact: Significantly reduced memory allocations per frame by avoiding string copies for items without text/description. Reduced arithmetic operations per tile by ~50% in the hot loop.
Learning: `std::string` return by value in accessor methods (`getText`) can be a silent killer in tight loops. Combining visibility checks with coordinate calculation avoids redundant math.

## 2024-10-27 - Optimization of SpatialHashGrid Viewport Traversal
Finding: `visitLeavesByViewport` had a cache thrashing issue where iterating `cx` (inner loop) reset the single-entry cache for every column, causing redundant `unordered_map` lookups for every cell in a row.
Impact: Reduced map lookups by ~16x (NODES_PER_CELL) in viewport traversal. Measured 2x speedup (13ms -> 6ms) in isolated benchmark for dense viewport traversal.
Learning: Single-entry caching in nested loops must respect the iteration order. Pre-fetching data for the inner loop (row-based) is more cache-friendly than random lookups.

## 2026-02-13 - Optimization of Sprite File Path Access
Finding: `GraphicManager::getSpriteFile()` returned `std::string` by value, causing a heap allocation and copy for every item in the render loop during sprite preloading checks.
Impact: Reduced memory allocations per frame significantly (thousands of allocations avoided per frame).
Learning: Returning `std::string` by value from a getter called in a hot loop is a major performance killer. Always use `const std::string&` or `std::string_view` for members.

## 2024-10-27 - Optimization of Tooltip Generation in Render Loop
Finding: `TileRenderer::DrawTile` generated full tooltip data (string allocations, layout calculations) for *every* item on a tile, causing massive CPU overhead (esp. `FillItemTooltipData`) even for items completely covered by others.
Impact: Reduced tooltip generation overhead from O(items_per_tile) to O(1) per tile. Eliminated thousands of redundant string allocations and layout passes per frame.
Learning: Even if a function (like `FillItemTooltipData`) is efficient, calling it inside a hot loop for invisible items is a major bottleneck. Pre-checking visibility/relevance (e.g. "is this the top item?") is crucial before doing expensive UI data preparation.
