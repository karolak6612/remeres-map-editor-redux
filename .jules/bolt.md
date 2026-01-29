## 2025-02-18 - Rendering Bottleneck: Per-Tile Logic
Learning: `TileRenderer::DrawTile` executes per visible tile every frame. Tooltips are currently drawn for all items, which is intentional (labels), but this design choice inherently limits performance on large views due to string operations.
Action: Future optimizations for tooltips should focus on caching the generated strings or using a more efficient font renderer, rather than culling them, as culling contradicts the "label" behavior.

## 2025-02-18 - Safety vs Optimization in Data Transfer
Learning: Attempted to optimize `TooltipData` by changing `std::string` to `std::string_view` for `itemName`, assuming it pointed to static data in `g_items`. However, the code reviewer correctly identified that `CreateItemTooltipData` constructs `itemName` as a local `std::string_view` from logic that might involve temporary strings or just general fragility if the source isn't guaranteed stable. Storing `string_view` in a struct passed between rendering stages creates a high risk of Use-After-Free if the lifetime isn't strictly controlled.
Action: When optimizing data transfer structs, prefer `std::string` with move semantics (`std::move`) over `std::string_view` unless the source lifetime is undeniably static or strictly scoped (e.g., function parameters). Safety first.
