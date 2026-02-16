# Building RME

## Prerequisites

| Tool | Windows | Linux (Ubuntu) |
|------|---------|----------------|
| C++ Compiler | Visual Studio 2022 | GCC 11+ or Clang 14+ |
| CMake | 3.23+ | 3.23+ |
| Package Manager | vcpkg | Conan 2.x |

---

## Windows Setup

### 1. Install Visual Studio 2022
Download from [visualstudio.microsoft.com](https://visualstudio.microsoft.com/) with **Desktop development with C++** workload.

### 2. Install CMake
```cmd
winget install Kitware.CMake
```

### 3. Install vcpkg
```cmd
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
```

### 4. Build
```cmd
build_windows.bat
```
This script will:
1. Configure the project using CMake and vcpkg.
2. **Regenerate the Visual Studio solution** in `build\rme.sln` using CMake presets.
3. Perform a parallel build of the project.

**Project Output:** `build\Release\rme.exe`  
**VS Solution:** `build\rme.sln` (Use this file for development)  
**Build Log:** `build.log`

---

## Standalone Visual Studio Workspace

If you prefer to work natively in Visual Studio 2022 without using the automated build scripts, you can generate a dedicated workspace:

1.  Run `generate_vcproj.bat`.
2.  Open `vcproj\rme.sln`.

This will create a permanent solution in the `vcproj\ ` folder that is fully configured with all dependencies.

> [!TIP]
> Use `build_windows.bat` for quick automated builds and `generate_vcproj.bat` when you want a persistent IDE environment.

> [!IMPORTANT]
> The solution is now automatically managed by CMake. You can open `build/rme.sln` or `vcproj/rme.sln` directly to work on the project, and it will stay in sync with the source code.

---

## Linux (Ubuntu) Setup

### 1. Install Build Tools & wxWidgets
```bash
sudo apt update
sudo apt install -y build-essential cmake git python3 python3-pip ninja-build
sudo apt install -y libgtk-3-dev libgl1-mesa-dev libglu1-mesa-dev libwxgtk3.2-dev
```

### 2. Install Conan
```bash
pip3 install conan
export PATH="$HOME/.local/bin:$PATH"  # Add to ~/.bashrc
```

### 3. Build
```bash
chmod +x build_linux.sh
./build_linux.sh
```
Output: `build_conan/build/rme`  
Log: `build.log`
