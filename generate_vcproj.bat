@echo off
REM ========================================
REM RME Standalone VS Solution Generator
REM Generates a full solution in vcproj/
REM ========================================

setlocal enabledelayedexpansion

set SCRIPT_DIR=%~dp0
set IDE_DIR=%SCRIPT_DIR%vcproj
set VCPKG_ROOT=c:\vcpkg

echo [1/2] Creating vcproj directory...
if not exist "%IDE_DIR%" mkdir "%IDE_DIR%"

echo [2/2] Generating Visual Studio 2022 solution...
cmake -S "%SCRIPT_DIR%." -B "%IDE_DIR%" -G "Visual Studio 17 2022" -A x64 "-DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"

if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: Solution generation failed.
    pause
    exit /b 1
)

echo.
echo ========================================
echo  SUCCESS: Solution generated in vcproj/
echo  Open: %IDE_DIR%\rme.sln
echo ========================================
echo.
pause

endlocal
