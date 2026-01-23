import os
import re

def modernize_typedef(filepath):
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            lines = f.readlines()
    except UnicodeDecodeError:
        print(f"Skipping {filepath} due to encoding.")
        return

    new_lines = []
    # Regex to match single-line typedefs
    # Capture group 1: Type (can be complex, greedy)
    # Capture group 2: Alias (simple identifier, at end of statement)
    # We use rstrip to handle trailing whitespace/comments if simple

    # Improved regex:
    # ^\s*typedef\s+ : Starts with typedef
    # (.+) : The type definition (greedy)
    # \s+ : Space separator
    # (\w+) : The alias
    # ; : Semicolon
    pattern = re.compile(r'^(\s*)typedef\s+(.+)\s+(\w+);')

    modified = False
    for line in lines:
        # filter out likely non-simple typedefs
        if "typedef" not in line:
            new_lines.append(line)
            continue

        # Skip function pointers: typedef void (*Func)(...);
        if "(*" in line:
             new_lines.append(line)
             continue

        # Skip struct definitions: typedef struct { ... } Name;
        if "struct" in line and "{" in line:
            new_lines.append(line)
            continue

        match = pattern.match(line)
        if match:
            indent = match.group(1)
            type_def = match.group(2).strip()
            alias = match.group(3).strip()

            # Handle cases where type_def ends with * that got attached to type_def or space
            # "typedef int *ptr;" -> type="int *", alias="ptr"
            # "typedef int* ptr;" -> type="int*", alias="ptr"

            new_line = f"{indent}using {alias} = {type_def};\n"
            new_lines.append(new_line)
            modified = True
            print(f"Replacing in {filepath}: {line.strip()} -> {new_line.strip()}")
        else:
            new_lines.append(line)

    if modified:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.writelines(new_lines)

def main():
    source_dir = "source/"
    print("Starting typedef modernization...")
    for root, dirs, files in os.walk(source_dir):
        if "ext" in root.split(os.sep):
            continue

        for file in files:
            if file.endswith(".cpp") or file.endswith(".h"):
                filepath = os.path.join(root, file)
                # Double check path
                if "source/ext/" in filepath.replace("\\", "/"):
                    continue
                modernize_typedef(filepath)
    print("Typedef modernization complete.")

if __name__ == "__main__":
    main()
