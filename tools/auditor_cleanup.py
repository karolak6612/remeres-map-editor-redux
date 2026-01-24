import os
import re

SOURCE_DIR = 'source'
EXCLUDE_DIR = 'source/ext'
EXCLUDE_TYPEDEF_FILE = 'source/mt_rand.cpp'

def process_file(filepath):
    with open(filepath, 'r') as f:
        lines = f.readlines()

    new_lines = []
    modified = False

    for line in lines:
        original_line = line

        # 1. Remove commented printf
        if re.match(r'^\s*//\s*printf', line):
            modified = True
            continue # Skip this line (delete it)

        # 2. Replace NULL with nullptr
        if 'NULL' in line:
            # Use regex to replace whole word NULL
            new_line = re.sub(r'\bNULL\b', 'nullptr', line)
            if new_line != line:
                line = new_line
                modified = True

        # 3. Convert typedef to using
        # Only if not in excluded file for typedefs
        if filepath != EXCLUDE_TYPEDEF_FILE and 'typedef' in line:
            # Match: typedef <type> <alias>;
            # Avoid structs: typedef struct ...
            if 'struct' not in line and '{' not in line and '}' not in line:
                # Handle function pointers or complex types separately or skip them
                # Our regex expects "typedef type alias;"
                # Regex explanation:
                # ^\s*typedef\s+ : start with typedef
                # (.+?) : type (non-greedy)
                # \s+ : space
                # (\w+) : alias
                # ;\s*$ : end with semicolon
                match = re.match(r'^\s*typedef\s+(.+?)\s+(\w+);\s*$', line)
                if match:
                    type_def = match.group(1).strip()
                    alias = match.group(2).strip()

                    # Extra safety: check for parentheses which might indicate function pointer
                    # e.g. typedef void (*func)(int);
                    # Also avoid "typedef enum" if not caught by struct check (though enum usually has enum keyword)
                    if 'enum' not in line and '(' not in type_def and ')' not in type_def:
                        # Keep indentation
                        indent = re.match(r'^\s*', line).group(0)
                        line = f'{indent}using {alias} = {type_def};\n'
                        modified = True

        new_lines.append(line)

    if modified:
        with open(filepath, 'w') as f:
            f.writelines(new_lines)
        print(f"Modified: {filepath}")

def main():
    for root, dirs, files in os.walk(SOURCE_DIR):
        # Modify dirs in-place to skip excluded directories during traversal
        # This is strictly not needed if we check path starts with, but good for optimization
        if 'ext' in dirs:
             # This only removes 'ext' if it is a direct child.
             # But our exclude logic is broader: source/ext/*
             pass

        for file in files:
            if file.endswith('.cpp') or file.endswith('.h'):
                filepath = os.path.join(root, file)
                # Check exclusion path
                if filepath.startswith(EXCLUDE_DIR):
                    continue

                process_file(filepath)

if __name__ == '__main__':
    main()
