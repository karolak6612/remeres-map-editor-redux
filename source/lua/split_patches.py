#!/usr/bin/env python3
"""
Script to split patch files into individual patches per file.
Skips lua_*.* files and saves each patch to a 'patches' folder.
"""

import re
import sys
import os
from pathlib import Path


def is_lua_api_file(filepath: str) -> bool:
    """
    Check if the file path matches lua_*.* pattern (e.g., lua_api.cpp, lua_api.h).
    """
    filename = Path(filepath).name
    return filename.startswith('lua_') and '.' in filename


def extract_filename_from_diff(diff_line: str) -> str:
    """Extract the filename from a diff --git line."""
    match = re.match(r'diff --git a/(.*) b/(.*)', diff_line)
    if match:
        return match.group(2)  # Use the 'b' path (new file path)
    return None


def split_patch_file(patch_content: str, patch_filename: str, output_dir: Path) -> int:
    """
    Split a patch file into individual patches per file.
    Skips lua_*.* files.
    
    Returns the number of patches created.
    """
    lines = patch_content.split('\n')
    
    # Extract base name for numbering
    base_name = Path(patch_filename).stem  # e.g., "first" from "first.patch"
    
    current_diff_lines = []
    patch_count = 0
    in_diff = False
    
    i = 0
    while i < len(lines):
        line = lines[i]
        
        if line.startswith('diff --git '):
            # Save previous diff if it exists and is not a lua file
            if current_diff_lines:
                filepath = extract_filename_from_diff(current_diff_lines[0])
                if filepath and not is_lua_api_file(filepath):
                    patch_count += 1
                    save_patch(current_diff_lines, output_dir, base_name, patch_count, filepath)
            
            # Start new diff
            current_diff_lines = [line]
            in_diff = True
            i += 1
        elif in_diff:
            current_diff_lines.append(line)
            i += 1
        else:
            # Header lines before first diff - skip or save as info
            i += 1
    
    # Don't forget the last diff
    if current_diff_lines:
        filepath = extract_filename_from_diff(current_diff_lines[0])
        if filepath and not is_lua_api_file(filepath):
            patch_count += 1
            save_patch(current_diff_lines, output_dir, base_name, patch_count, filepath)
    
    return patch_count


def save_patch(diff_lines: list, output_dir: Path, base_name: str, patch_num: int, filepath: str):
    """
    Save a single diff to a patch file.
    
    Filename format: {base_name}_{patch_num:03d}_{sanitized_filename}.patch
    """
    # Sanitize filename for use in patch filename
    # Replace / and \ with _, remove special chars
    sanitized = re.sub(r'[^\w\-.]', '_', filepath)
    sanitized = re.sub(r'_+', '_', sanitized)  # Collapse multiple underscores
    sanitized = sanitized.strip('_')
    
    # Create output filename
    output_filename = f"{base_name}_{patch_num:03d}_{sanitized}.patch"
    output_path = output_dir / output_filename
    
    # Write the patch
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write('\n'.join(diff_lines))
    
    print(f"  Created: {output_filename}")


def process_patch_file(input_path: str, output_base: Path) -> int:
    """
    Process a patch file and split it into individual patches.
    
    Returns the number of patches created.
    """
    input_file = Path(input_path)
    
    if not input_file.exists():
        print(f"Error: File not found: {input_path}")
        return 0
    
    # Read the patch file
    with open(input_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Create output directory
    output_dir = output_base / input_file.stem  # e.g., "patches/first"
    output_dir.mkdir(parents=True, exist_ok=True)
    
    print(f"\nProcessing: {input_path}")
    print(f"  Output directory: {output_dir}")
    
    # Split and save patches
    patch_count = split_patch_file(content, input_file.name, output_dir)
    
    print(f"  Created {patch_count} patch(es) (lua_*.* files skipped)")
    
    return patch_count


def main():
    """Main entry point."""
    if len(sys.argv) < 2:
        print("Usage:")
        print("  python split_patches.py <patch_file1> [patch_file2] ...")
        print("\nExamples:")
        print("  python split_patches.py first.patch")
        print("  python split_patches.py first.patch second.patch")
        print("\nOutput:")
        print("  Creates patches/<patch_name>_<num>_<filename>.patch files")
        print("  Skips all lua_*.* files")
        sys.exit(1)
    
    patch_files = sys.argv[1:]
    
    # Create main patches directory
    script_dir = Path(__file__).parent
    patches_dir = script_dir / "patches"
    patches_dir.mkdir(exist_ok=True)
    
    print(f"Output directory: {patches_dir}")
    
    total_patches = 0
    for patch_file in patch_files:
        if patch_file.endswith('.patch'):
            count = process_patch_file(patch_file, patches_dir)
            total_patches += count
        else:
            print(f"Skipping non-patch file: {patch_file}")
    
    print(f"\n{'='*50}")
    print(f"Total: {total_patches} patch file(s) created")


if __name__ == '__main__':
    main()
