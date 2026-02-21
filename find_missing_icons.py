import os
import re

def find_missing_icons(root_dir):
    # Regex to capture variable name in assignment: var = new(d) wxButton
    # Handles *var = ...
    button_regex = re.compile(r'(\w+)\s*=\s*newd?\s*wxButton\s*\(')
    # Regex to capture SetBitmap calls: var->SetBitmap
    bitmap_regex = re.compile(r'(\w+)->SetBitmap\s*\(')

    missing_icons = []

    for root, dirs, files in os.walk(root_dir):
        for file in files:
            if file.endswith('.cpp') or file.endswith('.h'):
                filepath = os.path.join(root, file)
                try:
                    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                        content = f.read()

                        # Find all buttons created
                        buttons = {} # name -> line_number
                        # We use finditer to locate all occurrences
                        for match in button_regex.finditer(content):
                            # Check if line is commented out (simplistic check)
                            line_start = content.rfind('\n', 0, match.start()) + 1
                            line_content = content[line_start:match.start()]
                            if '//' in line_content:
                                continue

                            btn_name = match.group(1)
                            buttons[btn_name] = content.count('\n', 0, match.start()) + 1

                        if not buttons:
                            continue

                        # Check if SetBitmap is called for them
                        # We need to find SetBitmap for THESE buttons
                        # But wait, SetBitmap might be called on a member variable not captured here if definition is separate.
                        # This script only checks local variables or members assigned in this file.

                        # Find all SetBitmap calls in the file
                        bitmaps_set = set()
                        for match in bitmap_regex.finditer(content):
                            bitmaps_set.add(match.group(1))

                        for btn, line in buttons.items():
                            if btn not in bitmaps_set:
                                missing_icons.append(f"{filepath}:{line} - {btn}")
                except Exception as e:
                    print(f"Error reading {filepath}: {e}")

    return missing_icons

if __name__ == "__main__":
    missing = find_missing_icons("source")
    for m in missing:
        print(m)
