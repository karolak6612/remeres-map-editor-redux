#!/bin/bash

# Define replacements
declare -A replacements=(
    ["asRaw"]="as<RAWBrush>"
    ["isRaw"]="is<RAWBrush>"
    ["asDoodad"]="as<DoodadBrush>"
    ["isDoodad"]="is<DoodadBrush>"
    ["asTerrain"]="as<TerrainBrush>"
    ["isTerrain"]="is<TerrainBrush>"
    ["asGround"]="as<GroundBrush>"
    ["isGround"]="is<GroundBrush>"
    ["asWall"]="as<WallBrush>"
    ["isWall"]="is<WallBrush>"
    ["asWallDecoration"]="as<WallDecorationBrush>"
    ["isWallDecoration"]="is<WallDecorationBrush>"
    ["asTable"]="as<TableBrush>"
    ["isTable"]="is<TableBrush>"
    ["asCarpet"]="as<CarpetBrush>"
    ["isCarpet"]="is<CarpetBrush>"
    ["asDoor"]="as<DoorBrush>"
    ["isDoor"]="is<DoorBrush>"
    ["asOptionalBorder"]="as<OptionalBorderBrush>"
    ["isOptionalBorder"]="is<OptionalBorderBrush>"
    ["asCreature"]="as<CreatureBrush>"
    ["isCreature"]="is<CreatureBrush>"
    ["asSpawn"]="as<SpawnBrush>"
    ["isSpawn"]="is<SpawnBrush>"
    ["asHouse"]="as<HouseBrush>"
    ["isHouse"]="is<HouseBrush>"
    ["asHouseExit"]="as<HouseExitBrush>"
    ["isHouseExit"]="is<HouseExitBrush>"
    ["asWaypoint"]="as<WaypointBrush>"
    ["isWaypoint"]="is<WaypointBrush>"
    ["asFlag"]="as<FlagBrush>"
    ["isFlag"]="is<FlagBrush>"
    ["asEraser"]="as<EraserBrush>"
    ["isEraser"]="is<EraserBrush>"
)

# Iterate over all source files
find source -name "*.cpp" -o -name "*.h" | while read -r file; do
    for key in "${!replacements[@]}"; do
        value="${replacements[$key]}"
        # Using a temporary file for sed replacement
        # Careful with escaping < and > in replacement string for sed
        # sed delimiter can be anything, e.g. |
        # We match ->key() and replace with ->value()
        sed -i "s/->${key}()/->${value}()/g" "$file"
    done
done
