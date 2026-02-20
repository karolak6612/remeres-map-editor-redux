import os
import re

def find_buttons_without_icons(root_dir):
    button_regex = re.compile(r'(\w+)\s*=\s*new\s+(?:d\s+)?wxButton\s*\(')
    bitmap_regex = re.compile(r'(\w+)->SetBitmap\s*\(')

    files_to_check = []
    for dirpath, dirnames, filenames in os.walk(root_dir):
        for filename in filenames:
            if filename.endswith('.cpp') or filename.endswith('.h'):
                files_to_check.append(os.path.join(dirpath, filename))

    for filepath in files_to_check:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()

        buttons = button_regex.findall(content)
        bitmaps = bitmap_regex.findall(content)

        missing_icons = []
        for btn in buttons:
            if btn not in bitmaps:
                missing_icons.append(btn)

        if missing_icons:
            print(f"File: {filepath}")
            for btn in missing_icons:
                print(f"  Button without icon: {btn}")

if __name__ == "__main__":
    find_buttons_without_icons("source/ui")
