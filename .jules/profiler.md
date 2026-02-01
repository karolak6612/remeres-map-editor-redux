## 2024-10-26 - Optimization of Item Tooltips and Viewport Culling
Finding: Generating tooltip data for every item in the viewport involved excessive string allocation and copying, even for items with no tooltip info. Additionally, the render loop calculated screen coordinates twice for every visible tile.
Impact: Significantly reduced memory allocations per frame by avoiding string copies for items without text/description. Reduced arithmetic operations per tile by ~50% in the hot loop.
Learning: `std::string` return by value in accessor methods (`getText`) can be a silent killer in tight loops. Combining visibility checks with coordinate calculation avoids redundant math.

## 2024-10-27 - Tooltip Generation and House Color Optimization
Finding: `TileRenderer` was allocating `std::string` and `std::vector` for tooltip data for *every* item in the viewport on the active floor, regardless of hover state. Also, house color was recalculated for every item on a house tile.
Impact: Eliminated thousands of allocations per frame by restricting tooltip generation to the hovered tile. Reduced house color calculation overhead by hoisting it out of the item loop. Stopped per-frame console logging in `SpriteDrawer`.
Learning: Unconditional data generation in render loops (especially involving heap allocation) is a major bottleneck. Always check if the data is actually needed (e.g., hover state) before generating it.
