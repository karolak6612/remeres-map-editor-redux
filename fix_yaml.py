with open('.github/workflows/build.yml', 'r') as f:
    content = f.read()

content = content.replace('libgtk-3-dev \\', 'libgtk-3-dev \\\n            libvulkan-dev \\')

with open('.github/workflows/build.yml', 'w') as f:
    f.write(content)
