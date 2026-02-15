import os
import re

def scan_file(filepath):
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            lines = f.readlines()
    except Exception as e:
        print(f"Error reading {filepath}: {e}")
        return

    file_length = len(lines)
    if file_length > 500:
        print(f"[Large Class/File] {filepath}: {file_length} lines")

    current_function = None
    function_start_line = 0
    brace_balance = 0
    in_function = False

    # Simple C++ function detection regex (return type, name, params)
    # This is a very rough heuristic
    func_pattern = re.compile(r'^\s*[\w\:<>]+[\*\&]*\s+([\w\:]+)\s*\((.*)\)\s*(const|noexcept|override|final)*\s*\{?')

    for i, line in enumerate(lines):
        line_stripped = line.strip()

        # Check for switch
        if re.search(r'\bswitch\s*\(', line_stripped):
             print(f"[Switch Statement] {filepath}:{i+1}")

        # Function detection heuristic
        match = func_pattern.match(line)
        if match and not line_stripped.startswith('if') and not line_stripped.startswith('for') and not line_stripped.startswith('while') and not line_stripped.startswith('switch') and ';' not in line_stripped:
            func_name = match.group(1)
            params = match.group(2)
            param_count = len(params.split(',')) if params.strip() else 0

            if param_count > 5:
                 print(f"[Long Parameter List] {filepath}:{i+1} Function '{func_name}' has {param_count} parameters")

            if not in_function:
                current_function = func_name
                function_start_line = i
                in_function = True
                brace_balance = 0

        if in_function:
            brace_balance += line.count('{')
            brace_balance -= line.count('}')

            if brace_balance <= 0 and line.count('}') > 0:
                func_length = i - function_start_line
                if func_length > 50:
                    print(f"[Long Method] {filepath}:{function_start_line+1} Function '{current_function}' is {func_length} lines long")
                in_function = False

def main():
    source_dir = 'source'
    for root, dirs, files in os.walk(source_dir):
        for file in files:
            if file.endswith('.cpp') or file.endswith('.h'):
                scan_file(os.path.join(root, file))

if __name__ == "__main__":
    main()
