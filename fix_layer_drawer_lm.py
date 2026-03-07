import os
import re

path = r'C:\Users\karol\Documents\GitHub\remeres-map-editor-redux\source\rendering\drawers\map_layer_drawer.cpp'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

content = re.sub(r'render_ctx\.editor\.live_manager', '(*render_ctx.live_manager)', content)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)
print("Updated map_layer_drawer.cpp")
