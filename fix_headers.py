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

base = r'C:\Users\karol\Documents\GitHub\remeres-map-editor-redux\source'

# BrushOverlayDrawer
bod_h = os.path.join(base, 'rendering', 'drawers', 'overlays', 'brush_overlay_drawer.h')
process_file(bod_h, [(r'Editor\s*&\s*editor', 'Map& map')])

# PreviewDrawer
pd_h = os.path.join(base, 'rendering', 'drawers', 'overlays', 'preview_drawer.h')
process_file(pd_h, [(r'Editor\s*&\s*editor', 'Map& map')])

# DragShadowDrawer
dsd_h = os.path.join(base, 'rendering', 'drawers', 'cursors', 'drag_shadow_drawer.h')
process_file(dsd_h, [(r'Editor\s*&\s*editor', 'Map& map, Selection& selection')])

# LiveCursorDrawer
lcd_h = os.path.join(base, 'rendering', 'drawers', 'cursors', 'live_cursor_drawer.h')
process_file(lcd_h, [(r'Editor\s*&\s*editor', 'LiveManager& live_manager')])

# MarkerDrawer
md_h = os.path.join(base, 'rendering', 'drawers', 'overlays', 'marker_drawer.h')
process_file(md_h, [(r'Editor\s*&\s*editor', 'Map& map')])

