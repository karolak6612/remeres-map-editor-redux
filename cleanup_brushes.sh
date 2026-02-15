#!/bin/bash

# List of files to modify
FILES=(
    "source/brushes/ground/ground_brush.h"
    "source/brushes/wall/wall_brush.h"
    "source/brushes/raw/raw_brush.h"
    "source/brushes/doodad/doodad_brush.h"
    "source/brushes/table/table_brush.h"
    "source/brushes/carpet/carpet_brush.h"
    "source/brushes/door/door_brush.h"
    "source/brushes/border/optional_border_brush.h"
    "source/brushes/creature/creature_brush.h"
    "source/brushes/spawn/spawn_brush.h"
    "source/brushes/house/house_brush.h"
    "source/brushes/house/house_exit_brush.h"
    "source/brushes/waypoint/waypoint_brush.h"
    "source/brushes/flag/flag_brush.h"
)

# Function to remove lines
remove_lines() {
    local file="$1"
    # Remove virtual bool isXXX() const { return true; }
    sed -i '/virtual bool is[a-zA-Z0-9_]*() const {/,/}/d' "$file"
    sed -i '/bool is[a-zA-Z0-9_]*() const override {/,/}/d' "$file"

    # Remove virtual XXXBrush* asXXX() override { return ...; }
    # This is trickier because it might span multiple lines
    sed -i '/[a-zA-Z0-9_]*Brush\* as[a-zA-Z0-9_]*() override {/,/}/d' "$file"

    # Clean up empty lines created by deletion (optional but good)
}

for file in "${FILES[@]}"; do
    if [ -f "$file" ]; then
        remove_lines "$file"
        echo "Modified $file"
    else
        echo "File not found: $file"
    fi
done
