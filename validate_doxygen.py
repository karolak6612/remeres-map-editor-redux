import os
import re
import sys

files_to_check = [
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

def validate_file(filepath):
    if not os.path.exists(filepath):
        print(f"File not found: {filepath}")
        return False

    with open(filepath, 'r') as f:
        content = f.read()

    errors = 0

    # Regex for /* ... @tag ... */ (multi-line)
    # We look for /* that is NOT followed by * (so exclude /**)
    # Then containing @param, @brief, @return, @file, @class
    tags = r'@(param|brief|return|file|class|enum|struct|union|typedef|var|fn)'

    # This pattern attempts to find /* ... @tag
    # We want to match /* followed by anything until @tag, but stopping at */ if no tag found first.
    # Actually simpler: Find all /* ... */ blocks. Check if they contain @tag.

    comment_pattern = re.compile(r'/\*([^*]|[\r\n]|(\*+([^*/]|[\r\n])))*\*+/')

    for match in comment_pattern.finditer(content):
        comment = match.group(0)
        if comment.startswith("/**"):
            continue # Correct style

        # Check for tags
        if re.search(tags, comment):
            print(f"Error in {filepath}: Found old-style comment with Doxygen tag at index {match.start()}")
            print(f"Context: {comment[:100]}...")
            errors += 1

    if errors == 0:
        print(f"Validated {filepath}: OK")
        return True
    else:
        print(f"Validated {filepath}: FAILED with {errors} errors")
        return False

success = True
for filepath in files_to_check:
    if not validate_file(filepath):
        success = False

if not success:
    sys.exit(1)
