
import os
import re

patterns = {
    'raw_loops': r'for\s*\([^;]+;[^;]+;[^)]+\)',
    'typedef': r'\btypedef\b',
    'NULL': r'\bNULL\b',
    'raw_new': r'\bnew\s+',
    'raw_delete': r'\bdelete\s+',
    'c_style_cast': r'\(\w+\s*\*\s*\)',  # simple pointer cast
}

def scan_file(filepath):
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    results = {}
    for name, pattern in patterns.items():
        matches = re.findall(pattern, content)
        if matches:
            results[name] = len(matches)
    return results

def main():
    source_dir = 'source'
    summary = {k: 0 for k in patterns}
    file_details = []

    for root, _, files in os.walk(source_dir):
        for file in files:
            if file.endswith(('.cpp', '.h')):
                filepath = os.path.join(root, file)
                results = scan_file(filepath)
                if results:
                    file_details.append((filepath, results))
                    for k, v in results.items():
                        summary[k] += v

    print("Summary:")
    for k, v in summary.items():
        print(f"  {k}: {v}")

    print("\nTop 10 files with most issues:")
    file_details.sort(key=lambda x: sum(x[1].values()), reverse=True)
    for filepath, results in file_details[:10]:
        print(f"  {filepath}: {results}")

if __name__ == "__main__":
    main()
