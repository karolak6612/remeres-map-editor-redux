import os
import re

def fix_content(content):
    # 1. Fix block comments starting with /* that contain @tag
    content = re.sub(r'(?m)^(\s*)/\*(\s*\n\s*\*\s*@)', r'\1/**\2', content)

    # 2. Fix single line comments: /* @tag ... */ -> /** @tag ... */
    content = re.sub(r'(?m)^(\s*)/\*(\s*@)', r'\1/**\2', content)

    # 3. Fix trailing comments on enums or members
    # Pattern: Code ending in , or ; then whitespace then /* then text then */ then EOL
    # We convert /* ... */ to ///< ...
    # We must ensure we don't match /** ... */ or /// ...

    def replace_trailing(match):
        prefix = match.group(1)
        comment_body = match.group(3).strip() # Fixed: Use group 3 for content
        return f'{prefix} ///< {comment_body}'

    # Regex:
    # Group 1: Non-comment content ending in , or ; and whitespace
    # Match /* but not /** (captured as group 2, which is just the first *)
    # Group 3: Comment body
    # Match */
    # End of line
    content = re.sub(r'([,;]\s*)/(\*)[^*](.*?)\*/\s*$', replace_trailing, content, flags=re.MULTILINE)

    return content

def main():
    root_dir = 'source'
    extensions = ('.h', '.cpp')

    for dirpath, dirnames, filenames in os.walk(root_dir):
        for filename in filenames:
            if filename.endswith(extensions):
                filepath = os.path.join(dirpath, filename)

                try:
                    with open(filepath, 'r', encoding='utf-8') as f:
                        content = f.read()

                    new_content = fix_content(content)

                    if new_content != content:
                        with open(filepath, 'w', encoding='utf-8') as f:
                            f.write(new_content)
                        print(f"Fixed: {filepath}")

                except Exception as e:
                    print(f"Error processing {filepath}: {e}")

if __name__ == '__main__':
    main()
