import os
import re

files_to_fix = [
    "source/map/tile.h",
    "source/map/map.h",
    "source/brushes/brush.h",
    "source/game/item.h",
    "source/io/iomap.h",
    "source/map/basemap.h",
    "source/map/position.h",
    "source/game/creature.h",
    "source/editor/editor.h",
    "source/editor/action.h",
    "source/map/map_region.h",
    "source/map/map_allocator.h",
    "source/map/map_search.h"
]

def fix_file(filepath):
    if not os.path.exists(filepath):
        print(f"File not found: {filepath}")
        return

    with open(filepath, 'r') as f:
        lines = f.readlines()

    new_lines = []
    i = 0
    while i < len(lines):
        line = lines[i]

        # 1. Check for `/**` line that might have caused broken indentation on the NEXT line
        match_start_new = re.match(r'^(\s*)/\*\*', line)

        # 2. Check for `/*` line that needs conversion
        match_start_old = re.match(r'^(\s*)/\*', line)

        if match_start_new:
            indent = match_start_new.group(1)
            # Check next line
            if i + 1 < len(lines):
                next_line = lines[i+1]
                # If next line starts with " * @" (broken indent from previous script)
                if next_line.startswith(" * @"):
                    lines[i+1] = indent + next_line

        elif match_start_old:
            indent = match_start_old.group(1)
            # Check if this is a doxygen block (next line has @tag)
            if i + 1 < len(lines):
                # We look for ` * @` with any indentation
                if re.search(r'^\s*\*\s*@', lines[i+1]):
                    # Convert to /**
                    line = line.replace('/*', '/**', 1)
                    # Ensure next line has correct indent
                    # If next line indentation matches `indent` + ` *`, good.
                    # If not, fix it.
                    next_line = lines[i+1]
                    next_match = re.match(r'^(\s*)\*\s*@', next_line)
                    if next_match:
                        current_next_indent = next_match.group(1)
                        if current_next_indent != indent + " ": # Assume 1 space align? Or same indent?
                            # Usually:
                            # /**
                            #  * @brief
                            # So indent should be `indent` + ` ` (space).
                            # Or if tabs, `\t` -> `\t `
                            pass

        new_lines.append(line)
        i += 1

    with open(filepath, 'w') as f:
        f.writelines(new_lines)
    print(f"Refixed {filepath}")

for filepath in files_to_fix:
    fix_file(filepath)
