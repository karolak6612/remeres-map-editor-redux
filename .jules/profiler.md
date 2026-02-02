## 2024-10-26 - Optimization of Item Tooltips and Viewport Culling
Finding: Generating tooltip data for every item in the viewport involved excessive string allocation and copying, even for items with no tooltip info. Additionally, the render loop calculated screen coordinates twice for every visible tile.
Impact: Significantly reduced memory allocations per frame by avoiding string copies for items without text/description. Reduced arithmetic operations per tile by ~50% in the hot loop.
Learning: `std::string` return by value in accessor methods (`getText`) can be a silent killer in tight loops. Combining visibility checks with coordinate calculation avoids redundant math.

## 2024-05-23 - Optimization of Preview Rendering and Tooltip Logic
Finding: `PreviewDrawer` was scanning the entire viewport pixels to find tiles in the secondary map (paste buffer), which is O(ScreenPixels) instead of O(PasteTiles). `TileRenderer` was generating tooltip data for every visible tile, causing massive string allocations per frame.
Impact: Reduced `PreviewDrawer` complexity from ~2M checks (1080p) to ~SizeOfPasteBuffer checks. Eliminated ~1000 allocations per frame by restricting tooltips to hovered tile.
Learning: Iterating sparse data structures (paste buffer) is always better than scanning the dense space (screen) to find them. "Show Tooltips" features often inadvertently mean "Generate All Tooltips" if not carefully gated by hover checks.
