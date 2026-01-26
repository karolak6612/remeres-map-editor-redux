## 2024-05-23 - Waypoint Tooltip Optimization
Finding: `TileRenderer::DrawTile` unconditionally called `editor->map.waypoints.getWaypoint(location)` for every tile, incurring a hash map lookup per tile per frame, regardless of whether a waypoint existed or tooltips were enabled.
Impact: Removed thousands of redundant hash map lookups per frame.
Learning: Always check existence flags (e.g., `getWaypointCount()`) before performing expensive lookups in hot render loops.
