# Profiler's Journal

## 2024-05-22 - Initial Performance Optimizations
Finding: CPU bottlenecks in redundant visibility checks and item iteration.
Impact: Expected reduction in frame time by eliminating redundant logic in hot paths.
Learning: `MapLayerDrawer` already iterates visible leaves, making `IsTileVisible` inside `DrawTile` redundant. Merging `AddLight` avoids double iteration over items.
