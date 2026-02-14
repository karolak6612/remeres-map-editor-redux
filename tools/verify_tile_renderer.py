import sys

def check_file(filepath):
    print(f"Checking {filepath}...")
    try:
        with open(filepath, 'r') as f:
            content = f.read()

        lines = content.split('\n')

        # Check for banned OpenGL calls inside loops (heuristic)
        banned_calls = [
            'glDrawArrays', 'glDrawElements',
            'glBindTexture',
            'glBufferSubData', 'glNamedBufferSubData'
        ]

        issues = []
        for i, line in enumerate(lines):
            # Simple check for calls, ignoring comments
            clean_line = line.split('//')[0].strip()
            for call in banned_calls:
                if call in clean_line:
                    # Heuristic: Check if inside a loop/drawing function.
                    # Since these files are dedicated to drawing, ANY call is suspicious if not batched.
                    # However, MapLayerDrawer might have legitimate setup calls, but inside Draw loop it shouldn't.
                    issues.append(f"Line {i+1}: Found banned call '{call}'")

        if issues:
            print(f"Found {len(issues)} issues in {filepath}:")
            for issue in issues:
                print(f"  - {issue}")
            return False
        else:
            print(f"Clean: {filepath}")
            return True

    except Exception as e:
        print(f"Error checking {filepath}: {e}")
        return False

def main():
    files_to_check = [
        'source/rendering/drawers/tiles/tile_renderer.cpp',
        'source/rendering/drawers/map_layer_drawer.cpp'
    ]

    all_pass = True
    for f in files_to_check:
        if not check_file(f):
            all_pass = False

    if all_pass:
        print("All checks passed! TileRenderer and MapLayerDrawer are clean.")
        sys.exit(0)
    else:
        print("Issues found.")
        sys.exit(1)

if __name__ == "__main__":
    main()
