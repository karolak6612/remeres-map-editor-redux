@echo off
REM ============================================================
REM  RME Windows Build Script
REM  Checks prerequisites, configures CMake, builds Release.
REM  Logs everything to build.log.
REM ============================================================

setlocal enabledelayedexpansion

REM --- Color helpers (Windows 10+ ANSI) ---
for /F %%a in ('echo prompt $E ^| cmd') do set "ESC=%%a"
set "GREEN=%ESC%[92m"
set "YELLOW=%ESC%[93m"
set "RED=%ESC%[91m"
set "CYAN=%ESC%[96m"
set "RESET=%ESC%[0m"
set "BOLD=%ESC%[1m"

set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR:~0,-1%"
set "BUILD_DIR=%PROJECT_ROOT%\build"
set "LOG_FILE=%PROJECT_ROOT%\build.log"
set "REQUIRED_CMAKE_MAJOR=3"
set "REQUIRED_CMAKE_MINOR=28"
set "TOTAL_CHECKS=7"

echo.
echo %BOLD%%CYAN%========================================================%RESET%
echo %BOLD%%CYAN%  RME Windows Build Script%RESET%
echo %BOLD%%CYAN%========================================================%RESET%
echo.

REM ==========================================================
REM  CHECK 1: CMake is installed and on PATH
REM ==========================================================
echo %BOLD%[1/%TOTAL_CHECKS%] Checking for CMake...%RESET%

where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo.
    echo   %RED%ERROR: CMake was not found on your PATH.%RESET%
    echo.
    echo   CMake is required to configure and build the project.
    echo   Please download and install it from:
    echo.
    echo     %CYAN%https://cmake.org/download/%RESET%
    echo.
    echo   During installation, make sure to select the option:
    echo     "Add CMake to the system PATH for all users"
    echo.
    echo   After installing, close this window and run this script again.
    echo.
    pause
    exit /b 1
)
echo   %GREEN%OK%RESET% - CMake found.

REM ==========================================================
REM  CHECK 2: CMake version >= 3.28
REM ==========================================================
echo %BOLD%[2/%TOTAL_CHECKS%] Checking CMake version ^(^>= %REQUIRED_CMAKE_MAJOR%.%REQUIRED_CMAKE_MINOR% required^)...%RESET%

for /f "tokens=3" %%v in ('cmake --version') do (
    if not defined CMAKE_VER set "CMAKE_VER=%%v"
)

if not defined CMAKE_VER (
    echo.
    echo   %RED%ERROR: Could not determine CMake version.%RESET%
    echo   %RED%       Please reinstall CMake from: https://cmake.org/download/%RESET%
    echo.
    pause
    exit /b 1
)

REM Parse major.minor
for /f "tokens=1,2 delims=." %%a in ("!CMAKE_VER!") do (
    set "CMAKE_MAJOR=%%a"
    set "CMAKE_MINOR=%%b"
)

set "CMAKE_OK=0"
if !CMAKE_MAJOR! gtr !REQUIRED_CMAKE_MAJOR! set "CMAKE_OK=1"
if !CMAKE_MAJOR! equ !REQUIRED_CMAKE_MAJOR! if !CMAKE_MINOR! geq !REQUIRED_CMAKE_MINOR! set "CMAKE_OK=1"

if "%CMAKE_OK%"=="0" (
    echo.
    echo   %RED%ERROR: CMake version is too old.%RESET%
    echo.
    echo     Your version:    %CMAKE_VER%
    echo     Required:        %REQUIRED_CMAKE_MAJOR%.%REQUIRED_CMAKE_MINOR%.0 or newer
    echo.
    echo   Please download the latest CMake from:
    echo.
    echo     %CYAN%https://cmake.org/download/%RESET%
    echo.
    echo   After updating, close this window and run this script again.
    echo.
    pause
    exit /b 1
)
echo   %GREEN%OK%RESET% - CMake !CMAKE_VER! found.

REM ==========================================================
REM  CHECK 3: Visual Studio C++ toolchain (latest version)
REM ==========================================================
echo %BOLD%[3/%TOTAL_CHECKS%] Checking for Visual Studio C++ toolchain...%RESET%

set "VS_FOUND=0"
set "VS_PATH="
set "VS_MAJOR="
set "VS_YEAR="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

REM Use vswhere to find the latest VS (any edition: Community, Pro, Build Tools)
if exist "%VSWHERE%" (
    REM Get installation path
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -property installationPath 2^>nul`) do (
        set "VS_PATH=%%i"
    )
    REM Get major version number (e.g. 17 for VS 2022, 18 for VS 2026)
    for /f "usebackq tokens=1 delims=." %%v in (`"%VSWHERE%" -latest -products * -property installationVersion 2^>nul`) do (
        set "VS_MAJOR=%%v"
    )
    REM Get product year (e.g. 2022, 2026)
    for /f "usebackq tokens=*" %%y in (`"%VSWHERE%" -latest -products * -property catalog_productLineVersion 2^>nul`) do (
        set "VS_YEAR=%%y"
    )
    if defined VS_MAJOR if defined VS_YEAR set "VS_FOUND=1"
)

REM Fallback: check MSBuild on PATH
if "!VS_FOUND!"=="0" (
    where msbuild >nul 2>&1
    if !ERRORLEVEL! equ 0 (
        for /f "tokens=1 delims=." %%m in ('msbuild -version -nologo 2^>nul') do (
            set "VS_MAJOR=%%m"
        )
        if defined VS_MAJOR (
            REM Map known major versions to years
            if "!VS_MAJOR!"=="16" set "VS_YEAR=2019"
            if "!VS_MAJOR!"=="17" set "VS_YEAR=2022"
            if "!VS_MAJOR!"=="18" set "VS_YEAR=2026"
            if defined VS_YEAR (
                set "VS_FOUND=1"
                set "VS_PATH=MSBuild !VS_MAJOR!.x on PATH"
            )
        )
    )
)

if "!VS_FOUND!"=="0" (
    echo.
    echo   %RED%ERROR: No Visual Studio C++ toolchain was found.%RESET%
    echo.
    echo   This project requires one of the following:
    echo     - Visual Studio 2022+ with "Desktop development with C++"
    echo     - Visual Studio Build Tools with C++ workload
    echo.
    echo   Please download and install from:
    echo.
    echo     %CYAN%https://visualstudio.microsoft.com/downloads/%RESET%
    echo.
    echo   After installing, close this window and run this script again.
    echo.
    pause
    exit /b 1
)
echo   %GREEN%OK%RESET% - Visual Studio !VS_YEAR! found ^(version !VS_MAJOR!^): !VS_PATH!

REM ==========================================================
REM  CHECK 4: Locate vcpkg root
REM ==========================================================
echo %BOLD%[4/%TOTAL_CHECKS%] Locating vcpkg...%RESET%

set "VCPKG_DIR="

REM Priority 1: VCPKG_ROOT environment variable
if defined VCPKG_ROOT (
    if exist "%VCPKG_ROOT%\.vcpkg-root" (
        set "VCPKG_DIR=%VCPKG_ROOT%"
        echo   Found via VCPKG_ROOT environment variable.
    )
)

REM Priority 2: Common install locations
if not defined VCPKG_DIR (
    for %%p in (
        "c:\vcpkg"
        "c:\src\vcpkg"
        "%USERPROFILE%\vcpkg"
        "%USERPROFILE%\source\repos\vcpkg"
    ) do (
        if not defined VCPKG_DIR (
            if exist "%%~p\.vcpkg-root" (
                set "VCPKG_DIR=%%~p"
                echo   Found at common location: %%~p
            )
        )
    )
)

if not defined VCPKG_DIR (
    echo.
    echo   %RED%ERROR: vcpkg was not found on this system.%RESET%
    echo.
    echo   vcpkg is the C++ package manager used to install dependencies.
    echo   To install it, open a command prompt and run:
    echo.
    echo     %CYAN%cd c:\%RESET%
    echo     %CYAN%git clone https://github.com/microsoft/vcpkg.git%RESET%
    echo     %CYAN%cd vcpkg%RESET%
    echo     %CYAN%bootstrap-vcpkg.bat%RESET%
    echo.
    echo   Then set the VCPKG_ROOT environment variable:
    echo.
    echo     %CYAN%setx VCPKG_ROOT "c:\vcpkg"%RESET%
    echo.
    echo   After installing, close this window and run this script again.
    echo.
    pause
    exit /b 1
)
echo   %GREEN%OK%RESET% - vcpkg found at: %VCPKG_DIR%

REM ==========================================================
REM  CHECK 5: vcpkg is bootstrapped (vcpkg.exe exists)
REM ==========================================================
echo %BOLD%[5/%TOTAL_CHECKS%] Checking vcpkg is bootstrapped...%RESET%

if not exist "%VCPKG_DIR%\vcpkg.exe" (
    echo   %YELLOW%vcpkg.exe not found. Running bootstrap...%RESET%
    echo.

    if not exist "%VCPKG_DIR%\bootstrap-vcpkg.bat" (
        echo   %RED%ERROR: bootstrap-vcpkg.bat not found in %VCPKG_DIR%.%RESET%
        echo   %RED%       Your vcpkg installation may be corrupt.%RESET%
        echo.
        echo   Please re-clone vcpkg:
        echo.
        echo     %CYAN%git clone https://github.com/microsoft/vcpkg.git %VCPKG_DIR%%RESET%
        echo.
        pause
        exit /b 1
    )

    call "%VCPKG_DIR%\bootstrap-vcpkg.bat" -disableMetrics
    if !ERRORLEVEL! neq 0 (
        echo.
        echo   %RED%ERROR: vcpkg bootstrap failed.%RESET%
        echo   %RED%       Please check the output above for details.%RESET%
        echo.
        pause
        exit /b 1
    )
    echo.
)
echo   %GREEN%OK%RESET% - vcpkg.exe is present.

REM ==========================================================
REM  CHECK 6: vcpkg integrate install
REM ==========================================================
echo %BOLD%[6/%TOTAL_CHECKS%] Running vcpkg integrate install...%RESET%

"%VCPKG_DIR%\vcpkg.exe" integrate install >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo   %YELLOW%WARNING: vcpkg integrate install failed.%RESET%
    echo   %YELLOW%         This may cause issues. Try running this script%RESET%
    echo   %YELLOW%         as Administrator if you haven't already.%RESET%
    echo.
) else (
    echo   %GREEN%OK%RESET% - vcpkg integration is active.
)

REM ==========================================================
REM  CHECK 7: vcpkg baseline freshness
REM ==========================================================
echo %BOLD%[7/%TOTAL_CHECKS%] Checking vcpkg baseline freshness...%RESET%

REM Parse the builtin-baseline commit hash from vcpkg.json
set "REQUIRED_BASELINE="
for /f "tokens=2 delims=:" %%a in ('findstr /C:"builtin-baseline" "!PROJECT_ROOT!\vcpkg.json"') do (
    set "RAW=%%a"
)
REM Strip quotes, spaces, and trailing characters
set "RAW=%RAW: =%"
set "RAW=%RAW:"=%"
set "RAW=%RAW:,=%"

if not "%RAW%"=="" (
    set "REQUIRED_BASELINE=%RAW%"
)

if defined REQUIRED_BASELINE (
    REM Check if the required baseline commit exists in the local vcpkg repo
    git -C "%VCPKG_DIR%" cat-file -t %REQUIRED_BASELINE% >nul 2>&1
    if !ERRORLEVEL! neq 0 (
        echo.
        echo   %RED%ERROR: Your vcpkg installation is outdated.%RESET%
        echo.
        echo   This project requires vcpkg baseline commit:
        echo     %CYAN%%REQUIRED_BASELINE%%RESET%
        echo.
        echo   but your local vcpkg repo does not contain this commit.
        echo.
        echo   To update, run:
        echo.
        echo     %CYAN%cd /d "%VCPKG_DIR%"%RESET%
        echo     %CYAN%git pull%RESET%
        echo.
        echo   After updating, close this window and run this script again.
        echo.
        pause
        exit /b 1
    )

    REM Verify the commit is an ancestor of HEAD (i.e., reachable)
    git -C "%VCPKG_DIR%" merge-base --is-ancestor %REQUIRED_BASELINE% HEAD >nul 2>&1
    if !ERRORLEVEL! neq 0 (
        echo.
        echo   %RED%ERROR: Your vcpkg installation is outdated.%RESET%
        echo.
        echo   The required baseline commit %CYAN%%REQUIRED_BASELINE%%RESET%
        echo   is NOT an ancestor of your current vcpkg HEAD.
        echo.
        echo   To update, run:
        echo.
        echo     %CYAN%cd /d "%VCPKG_DIR%"%RESET%
        echo     %CYAN%git pull%RESET%
        echo.
        echo   After updating, close this window and run this script again.
        echo.
        pause
        exit /b 1
    )
    echo   %GREEN%OK%RESET% - vcpkg baseline %REQUIRED_BASELINE:~0,12%... is available.
) else (
    echo   %YELLOW%WARNING: Could not parse builtin-baseline from vcpkg.json.%RESET%
    echo   %YELLOW%         Skipping baseline check. CMake may fail later.%RESET%
)

REM ==========================================================
REM  All checks passed — start building
REM ==========================================================
echo.
echo %BOLD%%CYAN%========================================================%RESET%
echo %BOLD%%CYAN%  All checks passed! Starting build...%RESET%
echo %BOLD%%CYAN%========================================================%RESET%
echo.

REM --- Init log file ---
echo ======================================== > "%LOG_FILE%"
echo  RME Windows Build Script >> "%LOG_FILE%"
echo  Started: %date% %time% >> "%LOG_FILE%"
echo  VS: !VS_YEAR! ^(version !VS_MAJOR!^) >> "%LOG_FILE%"
echo  vcpkg: !VCPKG_DIR! >> "%LOG_FILE%"
echo ======================================== >> "%LOG_FILE%"

REM ==========================================================
REM  Step 1/3: Configure CMake
REM ==========================================================
set "VS_GENERATOR=Visual Studio !VS_MAJOR! !VS_YEAR!"
echo %BOLD%[Build 1/3]%RESET% Configuring CMake with !VS_GENERATOR!...
echo [1/3] Configuring CMake with !VS_GENERATOR!... >> "%LOG_FILE%"

if not exist "!BUILD_DIR!" mkdir "!BUILD_DIR!"

cmake -S "!PROJECT_ROOT!" -B "!BUILD_DIR!" -G "!VS_GENERATOR!" -A x64 "-DCMAKE_TOOLCHAIN_FILE=!VCPKG_DIR!\scripts\buildsystems\vcpkg.cmake" "-DVCPKG_TARGET_TRIPLET=x64-windows" >> "%LOG_FILE%" 2>&1

if !ERRORLEVEL! neq 0 (
    echo.
    echo   %RED%ERROR: CMake configuration failed.%RESET%
    echo.
    echo   Common causes:
    echo     1. %BOLD%Missing vcpkg packages%RESET% - Try:
    echo          %CYAN%"!VCPKG_DIR!\vcpkg.exe" install --triplet x64-windows%RESET%
    echo     2. %BOLD%Stale CMake cache%RESET% - Try:
    echo          %CYAN%rmdir /s /q "!BUILD_DIR!"%RESET%
    echo     3. %BOLD%Toolchain mismatch%RESET% - Don't mix VS installations.
    echo.
    echo   See %CYAN%!LOG_FILE!%RESET% for full output.
    echo.
    pause
    exit /b 1
)
echo   %GREEN%OK%RESET% - Configuration complete.

REM ==========================================================
REM  Step 2/3: Build Release
REM ==========================================================
echo %BOLD%[Build 2/3]%RESET% Building Release...
echo [2/3] Building Release... >> "%LOG_FILE%"

cmake --build "!BUILD_DIR!" --config Release --target rme --parallel >> "%LOG_FILE%" 2>&1

if !ERRORLEVEL! neq 0 (
    echo.
    echo   %RED%ERROR: Build failed.%RESET%
    echo.
    echo   Common causes:
    echo     1. %BOLD%Compiler errors%RESET% - Check the log for C++ errors.
    echo     2. %BOLD%Missing dependencies%RESET% - A vcpkg package may have
    echo        failed to install. Re-run this script.
    echo     3. %BOLD%Disk space%RESET% - Ensure you have enough free space.
    echo.
    echo   See %CYAN%!LOG_FILE!%RESET% for full compiler output.
    echo.
    pause
    exit /b 1
)
echo   %GREEN%OK%RESET% - Build complete.

REM ==========================================================
REM  Step 3/3: Done!
REM ==========================================================
echo %BOLD%[Build 3/3]%RESET% Verifying output...

if not exist "!BUILD_DIR!\Release\rme.exe" (
    echo.
    echo   %YELLOW%WARNING: rme.exe was not found at expected location.%RESET%
    echo   %YELLOW%         Build may have placed it elsewhere.%RESET%
    echo   %YELLOW%         Check: !BUILD_DIR!%RESET%
    echo.
) else (
    echo   %GREEN%OK%RESET% - rme.exe found.
)

REM --- Finalize log ---
echo ======================================== >> "%LOG_FILE%"
echo  BUILD SUCCESSFUL >> "%LOG_FILE%"
echo  Output: !BUILD_DIR!\Release\rme.exe >> "%LOG_FILE%"
echo  Finished: %date% %time% >> "%LOG_FILE%"
echo ======================================== >> "%LOG_FILE%"

echo.
echo %GREEN%========================================================%RESET%
echo %GREEN%  BUILD SUCCESSFUL!%RESET%
echo %GREEN%========================================================%RESET%
echo.
echo   Output:    %BOLD%!BUILD_DIR!\Release\rme.exe%RESET%
echo   Solution:  !BUILD_DIR!\rme.sln
echo   Log:       !LOG_FILE!
echo   vcpkg:     !VCPKG_DIR!
echo.
pause

endlocal
