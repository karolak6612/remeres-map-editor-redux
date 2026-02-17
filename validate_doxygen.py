import os
import re
import sys

def validate_content(filepath, content):
    errors = 0
    # Optimized regex for C-style comments using dotall
    # Matches /* followed by anything (non-greedy) until */
    comment_pattern = re.compile(r'/\*.*?\*/', re.DOTALL)

    tags_pattern = re.compile(r'@(param|brief|return|file|class|enum|struct|union|typedef|var|fn|see|note|warning)')

    for match in comment_pattern.finditer(content):
        comment = match.group(0)

        if comment.startswith("/**"):
            continue

        if tags_pattern.search(comment):
            # Calculate line number
            lineno = content[:match.start()].count('\n') + 1
            print(f"Error in {filepath}:{lineno}: Found old-style comment '/*' with Doxygen tag.")
            # print(f"Context: {comment[:50]}...")
            errors += 1

    return errors

def main():
    root_dir = 'source'
    extensions = ('.h', '.cpp')
    total_errors = 0

    # Pre-compile to check for obvious non-issues faster?
    # No, compiling once is enough.

    file_count = 0
    for dirpath, dirnames, filenames in os.walk(root_dir):
        for filename in filenames:
            if filename.endswith(extensions):
                filepath = os.path.join(dirpath, filename)
                file_count += 1
                try:
                    with open(filepath, 'r', encoding='utf-8') as f:
                        content = f.read()
                    total_errors += validate_content(filepath, content)
                except Exception as e:
                    print(f"Error processing {filepath}: {e}")

    print(f"Scanned {file_count} files.")
    if total_errors > 0:
        print(f"Validation FAILED: Found {total_errors} errors.")
        sys.exit(1)
    else:
        print("Validation PASSED: No issues found.")

if __name__ == '__main__':
    main()
