#!/usr/bin/env python3
"""
Fixes include paths after moving rendering files to source/rendering/
Run from repository root: python scripts/fix_rendering_includes.py
"""

import os
from pathlib import Path

# Files being moved to rendering/
MOVED_HEADERS = [
    "graphics.h",
    "map_drawer.h", 
    "map_display.h",
    "light_drawer.h",
    "minimap_window.h",
    "sprites.h",
]

# Directories to process
SOURCE_DIR = Path("source")
RENDERING_DIR = SOURCE_DIR / "rendering"

def fix_includes_in_file(filepath: Path) -> int:
    """Fix includes in a single file. Returns count of changes made."""
    changes = 0
    
    with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()
    
    original = content
    
    # Check if this file is IN the rendering folder
    is_in_rendering = RENDERING_DIR in filepath.parents or filepath.parent == RENDERING_DIR
    
    for header in MOVED_HEADERS:
        old_include = f'#include "{header}"'
        
        if is_in_rendering:
            # Files in rendering/ keep relative includes (same folder)
            new_include = f'#include "{header}"'
        else:
            # Files outside rendering/ need the prefix
            new_include = f'#include "rendering/{header}"'
        
        if old_include in content and old_include != new_include:
            content = content.replace(old_include, new_include)
            changes += 1
            print(f"  {filepath}: {old_include} -> {new_include}")
    
    if content != original:
        with open(filepath, "w", encoding="utf-8") as f:
            f.write(content)
    
    return changes

def main():
    print("Fixing rendering includes...")
    total_changes = 0
    
    # Process all .cpp and .h files in source/
    for ext in ["*.cpp", "*.h"]:
        for filepath in SOURCE_DIR.rglob(ext):
            changes = fix_includes_in_file(filepath)
            total_changes += changes
    
    print(f"\nDone! Made {total_changes} include path updates.")

if __name__ == "__main__":
    main()
