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

### 1. Install Dependencies
You can use the optimized setup script which handles both `apt` and `Conan` dependencies:

```bash
chmod +x setup_conan.sh
./setup_conan.sh
```

> [!NOTE]
> This script is optimized for **Jules AI** and low-resource environments. It uses local gitignored directories for logs and ensures that only necessary binary libraries are downloaded.
> Since Linux only fetches a few small libraries from Conan (the rest are from `apt`), it uses `--build=missing` to guarantee a successful run without wasting time.

### 2. Build

#### Option A: Optimized Clang Build (Recommended for AI/Jules)
This is the fastest build path, using `Clang`, `Ninja`, and `Mold`. It is specifically optimized to run on 8GB RAM without timing out.

```bash
chmod +x build_clang.sh
./build_clang.sh
```
- **Mode:** Debug (`-O0 -g0` for maximum speed)
- **Parallelism:** Limited to 2 jobs to prevent OOM.
- **Output:** `build_clang/build/Debug/rme`
- **Log:** `build_clang/build_clang.log` (Errors are also printed to screen)

#### Option B: Standard GCC Build
```bash
chmod +x linux_build.sh
./linux_build.sh
```
- **Output:** `linux_build/build/Release/rme`
- **Log:** `linux_build/build_linux.log`

---

## Technical Notes for Jules AI
- Use `setup_conan.sh` for initial environment setup.
- Always use `build_clang.sh` for compilation. It is designed to be "blazing fast" and "token efficient" by only echoing errors to the terminal.
- Optimized scripts use gitignored directories (`build_conan/` and `build_clang/`) to ensure the workspace remains clean for integrity checks. Standard builds use `linux_build/`.
