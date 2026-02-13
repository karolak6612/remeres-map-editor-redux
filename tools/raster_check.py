import os
import re

def scan_file(filepath):
    issues = []
    with open(filepath, 'r') as f:
        lines = f.readlines()

    brace_level = 0
    loop_stack = [] # Stack of brace levels where loops started (the level *before* the loop body)

    for i, line in enumerate(lines):
        original_line = line
        # Remove comments for logic processing
        line = re.sub(r'//.*', '', line)

        # Tokenize important symbols
        # We need to process them in order
        tokens = [(m.start(), m.group()) for m in re.finditer(r'(for\b|while\b|\{|\})', line)]
        tokens.sort()

        # Determine if we are in a loop *at the start* of the line?
        # Or check if specific instruction is inside loop?
        # A simple "in_loop" flag for the line is good enough for finding calls.
        # But if the loop starts/ends on this line, we need to be careful.
        # Check instructions against current state.

        # To be precise, we should check instructions position relative to braces.
        # But that requires tokenizing instructions too.
        # Let's stick to "if loop_stack is not empty at start of line OR becomes not empty"
        # But if it ends on this line, we might flag something after the loop.

        # Better: iterate tokens. If we find an instruction token (glDraw...), check stack.

        # Let's add gl calls to tokens
        gl_pattern = r'\b(glDrawArrays|glDrawElements|glDrawRangeElements|glBindTexture|glBindTextureUnit|glBufferData|glBufferSubData|glNamedBufferSubData)\b'
        gl_matches = [(m.start(), m.group()) for m in re.finditer(gl_pattern, line)]

        all_tokens = tokens + gl_matches
        all_tokens.sort(key=lambda x: x[0])

        for pos, token in all_tokens:
            if token == '{':
                brace_level += 1
            elif token == '}':
                brace_level -= 1
                if loop_stack and brace_level == loop_stack[-1]:
                    loop_stack.pop()
            elif token == 'for' or token == 'while':
                # Assume the next block (brace_level + 1) is the loop body
                # We record the current brace_level (L). When we are at L+1, we are in loop.
                # When we return to L, loop ends.
                # However, we might not see '{' immediately.
                # But typically `for (...) {`.
                # If we push `brace_level` now, then when `{` happens, brace_level becomes L+1.
                # The condition "in loop" is `brace_level > loop_stack[-1]`.
                # Wait, if we have nested loops:
                # L=0. `for`. Stack=[0]. `{` -> L=1. In loop? 1 > 0. Yes.
                # L=1. `for`. Stack=[0, 1]. `{` -> L=2. In loop? 2 > 1. Yes.
                # L=2. `}` -> L=1. Stack top is 1. Pop? Yes. Stack=[0].
                # L=1. `}` -> L=0. Stack top is 0. Pop? Yes. Stack=[].

                # Handling single line `for (...) stmt;` without braces?
                # If no brace follows, this logic fails (stack never popped or popped at wrong }?).
                # Correct. This is a limitation. But we assume standard brace usage.
                # To be safer, we can check if '{' is not found soon... but that's complex.
                # Let's assume braces.
                loop_stack.append(brace_level)

            elif token.startswith('gl'):
                # Check if in loop
                is_in_loop = False
                if loop_stack:
                    # We are in loop if brace_level > loop_stack[-1]
                    # Example: `for (...) { glDraw... }`
                    # `for` pushed L. `{` inc L to L+1. `glDraw` sees L+1 > L. Correct.

                    # Example: `for (...) glDraw...;` (no braces)
                    # `for` pushed L. `glDraw` sees L. L is NOT > L.
                    # So we miss single line loops.
                    # We can relax to `>=`?
                    # If `>=`:
                    # `for (...) {` -> `for` pushes L. `{` incs to L+1.
                    # What if `for (...)` is followed by `glDraw` on same line before `{`?
                    # `for (...) glDraw... {` -> rare.
                    # But `for (...)` usually has condition inside `()`. `glDraw` not in `()`.
                    # So `glDraw` appears after `)`.

                    # If we use `>=`:
                    # `for` pushed 0. `glDraw` at 0. detected.
                    # `}` at 0 (from previous block? no).
                    # The issue is popping. If no brace, we never pop.
                    # Then everything after is "in loop". BAD.

                    # So sticking to `>` and braces is safer for false positives.
                    # We accept missing single-line loops to avoid flagging entire files.

                    if brace_level > loop_stack[-1]:
                        is_in_loop = True

                if is_in_loop:
                    # Classify issue
                    if 'glDraw' in token:
                        if 'Instanced' not in line: # Check whole line for Instanced context
                            issues.append({
                                'type': 'CRITICAL',
                                'message': 'Individual draw call inside loop',
                                'line': i + 1,
                                'content': original_line.strip()
                            })
                    elif 'glBindTexture' in token:
                        issues.append({
                            'type': 'HIGH',
                            'message': 'Texture binding inside loop',
                            'line': i + 1,
                            'content': original_line.strip()
                        })
                    elif 'glBuffer' in token or 'glNamedBuffer' in token:
                         issues.append({
                            'type': 'HIGH',
                            'message': 'Buffer update inside loop',
                            'line': i + 1,
                            'content': original_line.strip()
                        })

    return issues

def main():
    source_dir = 'source/rendering'
    all_issues = []
    scanned_files = 0

    for root, dirs, files in os.walk(source_dir):
        for file in files:
            if file.endswith('.cpp'):
                scanned_files += 1
                filepath = os.path.join(root, file)
                # Skip known batch implementation files
                if file in ['sprite_batch.cpp', 'primitive_renderer.cpp', 'light_drawer.cpp']:
                    continue

                try:
                    issues = scan_file(filepath)
                    for issue in issues:
                        issue['file'] = filepath
                        all_issues.append(issue)
                except Exception as e:
                    print(f"Error scanning {filepath}: {e}")

    # Print report
    print("OPENGL RENDERING SPECIALIST - Automated Report")
    print(f"Files Scanned: {scanned_files}")

    if not all_issues:
        print("No critical issues found.")
    else:
        for issue in all_issues:
            print(f"[{issue['type']}] {issue['message']}")
            print(f"  Location: {issue['file']}:{issue['line']}")
            print(f"  Code: {issue['content']}")
            print()

if __name__ == '__main__':
    main()
