## 2025-02-18 - Rendering Bottleneck: Per-Tile Logic
Learning: `TileRenderer::DrawTile` executes per visible tile every frame. Tooltips are currently drawn for all items, which is intentional (labels), but this design choice inherently limits performance on large views due to string operations.
Action: Future optimizations for tooltips should focus on caching the generated strings or using a more efficient font renderer, rather than culling them, as culling contradicts the "label" behavior.

## 2025-02-18 - Optimization Safety: Casting vs Allocations
Learning: Replaced `std::string` with `std::string_view` in `TooltipData` to eliminate thousands of allocations per frame in `DrawTile`. While `static_cast` offered minor CPU gains over `dynamic_cast`, it was reverted due to safety concerns regarding object type consistency (e.g. `setID` usage).
Action: Prioritize memory allocation reductions (O(N) allocations) over micro-optimizations like casting (O(N) RTTI checks) when safety is a concern.
