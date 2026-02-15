#!/bin/bash

# Define replacements for Item calls (reverting aggressive changes)
# item->is<WallBrush>() -> item->isWall()
# item->is<TableBrush>() -> item->isTable()
# item->is<CarpetBrush>() -> item->isCarpet()
# item->is<DoorBrush>() -> item->isDoor()

declare -A replacements=(
    ["is<WallBrush>"]="isWall"
    ["is<TableBrush>"]="isTable"
    ["is<CarpetBrush>"]="isCarpet"
    ["is<DoorBrush>"]="isDoor"
)

# Target specific variable names known to be Item*
VARS=("item" "i" "it" "iter" "wall")

for VAR in "${VARS[@]}"; do
    for key in "${!replacements[@]}"; do
        value="${replacements[$key]}"
        # Search for VAR->key() and replace with VAR->value()
        # Using grep to find files first to avoid unnecessary writes
        grep -r "${VAR}->${key}()" source | cut -d: -f1 | sort | uniq | while read -r file; do
             sed -i "s/${VAR}->${key}()/${VAR}->${value}()/g" "$file"
        done
    done
done
