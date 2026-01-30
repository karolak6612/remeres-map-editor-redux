import os
import re

def count_lines(filepath):
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
            return len(lines), lines
    except Exception as e:
        print(f"Error reading {filepath}: {e}")
        return 0, []

def analyze_file(filepath):
    line_count, lines = count_lines(filepath)
    if line_count == 0:
        return None

    functions = []
    current_func = None
    brace_balance = 0
    func_start = 0
<<<<<<< Updated upstream

    # Simple regex to detect function definitions (heuristic)
    # void Class::Method(...) {
    func_pattern = re.compile(r'^\s*[\w<>:*&]+\s+(\w+::)?\w+\s*\([^;]*\)\s*(const)?\s*\{')

    max_indent = 0

=======

    # Simple regex to detect function definitions (heuristic)
    # void Class::Method(...) {
    func_pattern = re.compile(r'^\s*[\w<>:*&]+\s+(\w+::)?\w+\s*\([^;]*\)\s*(const)?\s*\{')

    max_indent = 0

>>>>>>> Stashed changes
    for i, line in enumerate(lines):
        # Check nesting
        indent = len(line) - len(line.lstrip())
        if line.strip():
            max_indent = max(max_indent, indent)

        # Basic brace counting for function length
        if '{' in line:
            brace_balance += line.count('{')
            if brace_balance == 1 and func_pattern.match(line):
                current_func = line.strip()
                func_start = i
<<<<<<< Updated upstream

=======

>>>>>>> Stashed changes
        if '}' in line:
            brace_balance -= line.count('}')
            if brace_balance == 0 and current_func:
                length = i - func_start
                if length > 50:
                    functions.append((current_func, length, func_start + 1))
                current_func = None

    return {
        'filepath': filepath,
        'lines': line_count,
        'long_functions': functions,
        'max_indent': max_indent
    }

def main():
    source_dir = 'source'
    results = []
<<<<<<< Updated upstream

    for root, dirs, files in os.walk(source_dir):
        if 'ext' in root: # Skip external libraries
            continue

=======

    for root, dirs, files in os.walk(source_dir):
        if 'ext' in root: # Skip external libraries
            continue

>>>>>>> Stashed changes
        for file in files:
            if file.endswith('.cpp') or file.endswith('.h'):
                filepath = os.path.join(root, file)
                res = analyze_file(filepath)
                if res:
                    results.append(res)

    # Sort by file size
    results.sort(key=lambda x: x['lines'], reverse=True)
<<<<<<< Updated upstream

    print("TOP 10 LARGE FILES:")
    for r in results[:10]:
        print(f"{r['filepath']}: {r['lines']} lines")

=======

    print("TOP 10 LARGE FILES:")
    for r in results[:10]:
        print(f"{r['filepath']}: {r['lines']} lines")

>>>>>>> Stashed changes
    print("\nTOP 10 LONG FUNCTIONS:")
    all_funcs = []
    for r in results:
        for f in r['long_functions']:
            all_funcs.append((f[0], f[1], r['filepath'], f[2]))
<<<<<<< Updated upstream

=======

>>>>>>> Stashed changes
    all_funcs.sort(key=lambda x: x[1], reverse=True)
    for f in all_funcs[:10]:
        print(f"{f[2]}:{f[3]} - {f[0]} ({f[1]} lines)")

if __name__ == "__main__":
    main()
