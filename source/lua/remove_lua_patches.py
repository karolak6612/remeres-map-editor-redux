#!/usr/bin/env python3
"""
Script to remove all .lua file patches from patch files.
Keeps only patches for files OTHER than .lua files.
"""

import re
import sys
from pathlib import Path


def is_lua_api_file(filepath: str) -> bool:
    """
    Check if the file path matches lua_*.* pattern (e.g., lua_api.cpp, lua_api.h).
    """
    # Extract just the filename from the path
    filename = Path(filepath).name
    return filename.startswith('lua_') and '.' in filename


def remove_lua_patches(patch_content: str) -> str:
    """
    Remove all diff sections that target lua_*.* files from patch content.
    
    A diff section starts with 'diff --git' and ends before the next 'diff --git'
    or end of file.
    """
    lines = patch_content.split('\n')
    result_lines = []
    
    i = 0
    while i < len(lines):
        line = lines[i]
        
        # Check if this is the start of a diff section
        if line.startswith('diff --git '):
            # Collect all lines of this diff section
            diff_lines = [line]
            i += 1
            
            # Check if this diff involves any lua_*.* files
            # The diff --git line format is: diff --git a/path/to/file b/path/to/file
            has_lua_file = False
            
            # Extract file paths from the diff line
            match = re.match(r'diff --git a/(.*) b/(.*)', line)
            if match:
                file_a = match.group(1)
                file_b = match.group(2)
                if is_lua_api_file(file_a) or is_lua_api_file(file_b):
                    has_lua_file = True
            
            # Collect the rest of this diff section
            while i < len(lines):
                current_line = lines[i]
                
                # Check for --- line (another indicator of file path)
                if current_line.startswith('--- '):
                    if 'a/' in current_line or current_line.startswith('--- /dev/null'):
                        path_match = re.search(r'--- (?:a/)?(\S+)', current_line)
                        if path_match and is_lua_api_file(path_match.group(1)):
                            has_lua_file = True
                
                # Check for +++ line (another indicator of file path)
                if current_line.startswith('+++ '):
                    if 'b/' in current_line or current_line.startswith('+++ /dev/null'):
                        path_match = re.search(r'\+\+\+ (?:b/)?(\S+)', current_line)
                        if path_match and is_lua_api_file(path_match.group(1)):
                            has_lua_file = True
                
                diff_lines.append(current_line)
                
                # Check if we've reached the next diff section or end of file
                i += 1
                if i < len(lines) and lines[i].startswith('diff --git '):
                    break
            
            # Only keep this diff if it doesn't involve lua_*.* files
            if not has_lua_file:
                result_lines.extend(diff_lines)
                # Add a blank line between diffs if there's a next diff
                if i < len(lines) and lines[i].startswith('diff --git '):
                    result_lines.append('')
        else:
            # Keep header lines (Before the first diff --git)
            result_lines.append(line)
            i += 1
    
    return '\n'.join(result_lines)


def process_patch_file(input_path: str, output_path: str = None):
    """
    Process a patch file and remove all .lua file patches.
    
    Args:
        input_path: Path to the input patch file
        output_path: Path to the output patch file. If None, overwrites input file.
    """
    input_file = Path(input_path)
    
    if not input_file.exists():
        print(f"Error: File not found: {input_path}")
        return False
    
    # Read the patch file
    with open(input_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Remove .lua patches
    cleaned_content = remove_lua_patches(content)
    
    # Determine output path
    if output_path is None:
        output_file = input_file
    else:
        output_file = Path(output_path)
    
    # Write the cleaned patch
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(cleaned_content)
    
    # Calculate stats
    original_lines = content.count('\n')
    cleaned_lines = cleaned_content.count('\n')
    removed_lines = original_lines - cleaned_lines
    
    print(f"Processed: {input_path}")
    print(f"  Original lines: {original_lines}")
    print(f"  Cleaned lines: {cleaned_lines}")
    print(f"  Removed lines: {removed_lines}")
    if output_path:
        print(f"  Output: {output_path}")
    else:
        print(f"  Output: {input_path} (overwritten)")
    
    return True


def main():
    """Main entry point."""
    if len(sys.argv) < 2:
        print("Usage:")
        print("  python remove_lua_patches.py <patch_file> [output_file]")
        print("  python remove_lua_patches.py <patch_file1> <patch_file2> ...")
        print("\nExamples:")
        print("  python remove_lua_patches.py first.patch")
        print("  python remove_lua_patches.py first.patch first_cleaned.patch")
        print("  python remove_lua_patches.py *.patch")
        sys.exit(1)
    
    # Process all provided patch files
    patch_files = sys.argv[1:]
    
    # If exactly 2 args and second doesn't end with .patch, treat as input/output pair
    if len(patch_files) == 2 and not patch_files[1].endswith('.patch'):
        process_patch_file(patch_files[0], patch_files[1])
    else:
        # Process each file, overwriting in place
        for patch_file in patch_files:
            if patch_file.endswith('.patch'):
                process_patch_file(patch_file)
            else:
                print(f"Skipping non-patch file: {patch_file}")


if __name__ == '__main__':
    main()
