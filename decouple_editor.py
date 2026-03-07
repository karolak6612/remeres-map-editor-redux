import os
import re

def process_file(path, replacements):
    with open(path, 'r', encoding='utf-8') as f:
        content = f.read()
    new_content = content
    for pattern, repl in replacements:
        new_content = re.sub(pattern, repl, new_content)
    if new_content != content:
        print(f'Updated {path}')
        with open(path, 'w', encoding='utf-8') as f:
            f.write(new_content)

base = r'c:\Users\karol\Documents\GitHub\remeres-map-editor-redux\source'

# TileRenderer
tr_h = os.path.join(base, 'rendering', 'drawers', 'tiles', 'tile_renderer.h')
tr_cpp = os.path.join(base, 'rendering', 'drawers', 'tiles', 'tile_renderer.cpp')
process_file(tr_h, [
    (r'\bclass Editor;\b', 'class Map;\nclass Waypoint;'),
    (r'\bEditor&\s+editor;\b', 'Map& map;'),
    (r'\bEditor\*\s+editor;\b', 'Map* map;')
])
process_file(tr_cpp, [
    (r'\beditor\(&ctx\.editor\)\b', 'map(&ctx.map)'),
    (r'\beditor->map\.waypoints\b', 'map->waypoints'),
    (r',\s*\*editor\b', ', *map')
])

# MarkerDrawer
md_h = os.path.join(base, 'rendering', 'drawers', 'overlays', 'marker_drawer.h')
md_cpp = os.path.join(base, 'rendering', 'drawers', 'overlays', 'marker_drawer.cpp')
process_file(md_h, [
    (r'\bclass Editor;\b', 'class Map;'),
    (r'\bEditor&\s+editor\b', 'Map& map')
])
process_file(md_cpp, [
    (r'\bEditor\s+&editor\b', 'Map &map'),
    (r'\beditor\.map\b', 'map')
])

# BrushOverlayDrawer
bod_h = os.path.join(base, 'rendering', 'drawers', 'overlays', 'brush_overlay_drawer.h')
bod_cpp = os.path.join(base, 'rendering', 'drawers', 'overlays', 'brush_overlay_drawer.cpp')
process_file(bod_h, [
    (r'\bclass Editor;\b', 'class Map;'),
    (r'\bEditor\s+&editor\b', 'Map &map')
])
process_file(bod_cpp, [
    (r'\bEditor\s+&editor\b', 'Map &map'),
    (r'\beditor\.map\b', 'map')
])

# MinimapDrawer
mmd_h = os.path.join(base, 'rendering', 'drawers', 'minimap_drawer.h')
mmd_cpp = os.path.join(base, 'rendering', 'drawers', 'minimap_drawer.cpp')
process_file(mmd_h, [
    (r'\bclass Editor;\b', 'class Map;'),
    (r'\bEditor\s+&editor\b', 'Map &map')
])
process_file(mmd_cpp, [
    (r'\bEditor\s+&editor\b', 'Map &map'),
    (r'\beditor\.map\b', 'map')
])

# PreviewDrawer
pd_h = os.path.join(base, 'rendering', 'drawers', 'overlays', 'preview_drawer.h')
pd_cpp = os.path.join(base, 'rendering', 'drawers', 'overlays', 'preview_drawer.cpp')
process_file(pd_h, [
    (r'\bclass Editor;\b', 'class Map;'),
    (r'\bEditor\s+&editor\b', 'Map &map')
])
process_file(pd_cpp, [
    (r'\bEditor\s+&editor\b', 'Map &map'),
    (r'\beditor\.map\b', 'map')
])

# DragShadowDrawer
dsd_h = os.path.join(base, 'rendering', 'drawers', 'cursors', 'drag_shadow_drawer.h')
dsd_cpp = os.path.join(base, 'rendering', 'drawers', 'cursors', 'drag_shadow_drawer.cpp')
process_file(dsd_h, [
    (r'\bclass Editor;\b', 'class Map;\nclass Selection;'),
    (r'\bEditor\s+&editor\b', 'Map &map, Selection &selection')
])
process_file(dsd_cpp, [
    (r'\bEditor\s+&editor\b', 'Map &map, Selection &selection'),
    (r'\beditor\.map\b', 'map'),
    (r'\beditor\.selection\b', 'selection')
])

# LiveCursorDrawer
lcd_h = os.path.join(base, 'rendering', 'drawers', 'cursors', 'live_cursor_drawer.h')
lcd_cpp = os.path.join(base, 'rendering', 'drawers', 'cursors', 'live_cursor_drawer.cpp')
process_file(lcd_h, [
    (r'\bclass Editor;\b', 'class LiveManager;'),
    (r'\bEditor\s+&editor\b', 'LiveManager &live_manager')
])
process_file(lcd_cpp, [
    (r'\bEditor\s+&editor\b', 'LiveManager &live_manager'),
    (r'\beditor\.live_manager\b', 'live_manager')
])

# MapDrawer
mdr_cpp = os.path.join(base, 'rendering', 'map_drawer.cpp')
process_file(mdr_cpp, [
    (r'\.editor = editor,', '.map = editor.map,'),
    (r'drag_shadow_drawer->draw\(ctx, item_drawer\.get\(\), sprite_drawer\.get\(\),\s*creature_drawer\.get\(\), editor\);', 'drag_shadow_drawer->draw(ctx, item_drawer.get(), sprite_drawer.get(), creature_drawer.get(), editor.map, editor.selection);'),
    (r'live_cursor_drawer->draw\(ctx, editor\);', 'live_cursor_drawer->draw(ctx, editor.live_manager);'),
    (r'brush_overlay_drawer->draw\(ctx, item_drawer\.get\(\), sprite_drawer\.get\(\),\s*creature_drawer\.get\(\), editor\);', 'brush_overlay_drawer->draw(ctx, item_drawer.get(), sprite_drawer.get(), creature_drawer.get(), editor.map);'),
    (r'preview_drawer->draw\(ctx, floor_params, map_z, editor, item_drawer\.get\(\),\s*sprite_drawer\.get\(\), creature_drawer\.get\(\)\);', 'preview_drawer->draw(ctx, floor_params, map_z, editor.map, item_drawer.get(), sprite_drawer.get(), creature_drawer.get());')
])
