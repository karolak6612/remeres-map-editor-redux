## 2024-05-23 - Tooltip Generation Bottleneck
Finding: Tooltip generation logic was executed for every item on every visible tile every frame, even though only one tooltip is ever shown. This involved string allocations and database lookups for thousands of items per frame.
Impact: Significant CPU overhead removed.
Learning: The `DrawTile` function was being treated as a "render everything" loop without considering that tooltips are a singleton UI element. Moving logic to a "hover-only" path (`UpdateHoveredTooltip`) is the correct pattern.
