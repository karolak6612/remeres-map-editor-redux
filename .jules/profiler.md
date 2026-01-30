## 2024-10-26 - Optimization of Item Tooltips and Viewport Culling
Finding: Generating tooltip data for every item in the viewport involved excessive string allocation and copying, even for items with no tooltip info. Additionally, the render loop calculated screen coordinates twice for every visible tile.
Impact: Significantly reduced memory allocations per frame by avoiding string copies for items without text/description. Reduced arithmetic operations per tile by ~50% in the hot loop.
Learning: `std::string` return by value in accessor methods (`getText`) can be a silent killer in tight loops. Combining visibility checks with coordinate calculation avoids redundant math.

## 2024-05-22 - Fix Excessive Tooltip Generation and Coordinate Calculation
Finding: `TileRenderer::DrawTile` was generating complex tooltip data (string allocations, dynamic casts) for *every* visible item when `options.show_tooltips` was enabled, causing massive CPU/GPU overhead. Also, screen coordinates were recalculated per tile in `IsTileVisible`.
Impact: Tooltip generation restricted to hovered tile (O(N) -> O(1)). Coordinate calculation moved to incremental loop in callers, bypassing redundant `IsTileVisible` logic.
Learning: Always check for mouse hover condition before generating UI elements like tooltips in a render loop. Incremental coordinate calculation in tile loops is faster than per-tile projection.
