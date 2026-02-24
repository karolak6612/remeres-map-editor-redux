#!/bin/bash
# ========================================
# RME Linux Build Script (Conan)
# Always Release build, logs to build.log
# ========================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build_conan"
LOG_FILE="$SCRIPT_DIR/build.log"

{
    echo "========================================"
    echo " RME Linux Build Script"
    echo " Started: $(date)"
    echo "========================================"
} > "$LOG_FILE"

echo "[0/4] Verifying Conan installation..."
if ! command -v conan &> /dev/null; then
    echo "========================================"
    echo " ❌ ERROR: Conan not found in PATH"
    echo "========================================"
    echo " If you just ran setup_conan.sh, you may need to:"
    echo " 1. Restart your terminal"
    echo " 2. OR run: source ~/.profile"
    echo " 3. OR add ~/.local/bin to your PATH manually"
    echo "========================================"
    exit 1
fi

echo "[1/4] Checking Conan profile..."
echo "[1/4] Checking Conan profile..." >> "$LOG_FILE"

if ! conan profile show &> /dev/null; then
    echo "  Creating default Conan profile..."
    conan profile detect >> "$LOG_FILE" 2>&1
fi

# Detect GCC version — Conan Center prebuilt binaries max out at GCC 14.
# If the system GCC is newer, override to 14 (ABI-compatible via libstdc++11)
# so Conan downloads prebuilt packages instead of compiling from source.
MAX_CONAN_GCC=14
GCC_VERSION=$(gcc -dumpversion | cut -d. -f1)
CONAN_COMPILER_FLAG=""
if [ "$GCC_VERSION" -gt "$MAX_CONAN_GCC" ] 2>/dev/null; then
    echo "  GCC $GCC_VERSION detected (Conan prebuilts max: $MAX_CONAN_GCC), overriding compiler.version=$MAX_CONAN_GCC"
    echo "  GCC $GCC_VERSION > $MAX_CONAN_GCC, using -s compiler.version=$MAX_CONAN_GCC" >> "$LOG_FILE"
    CONAN_COMPILER_FLAG="-s compiler.version=$MAX_CONAN_GCC"
fi

echo "[2/4] Installing dependencies with Conan..."
echo "[2/4] Installing dependencies..." >> "$LOG_FILE"

RAW_LOG="$SCRIPT_DIR/build_raw.log"
> "$RAW_LOG"

conan install "$SCRIPT_DIR" -of "$BUILD_DIR" --build=missing -s build_type=Release $CONAN_COMPILER_FLAG -c tools.system.package_manager:mode=install -c tools.system.package_manager:sudo=True >> "$RAW_LOG" 2>&1

echo "[3/4] Configuring CMake with Ninja..."
echo "[3/4] Configuring CMake with Ninja..." >> "$LOG_FILE"

cmake --preset conan-release >> "$RAW_LOG" 2>&1

echo "[4/4] Building Release with Ninja..."
echo "[4/4] Building Release with Ninja..." >> "$LOG_FILE"

set +e
cmake --build --preset conan-release --parallel "$(nproc)" >> "$RAW_LOG" 2>&1
BUILD_EXIT=$?
set -e

# Filter: keep only warnings, errors, and link status from the raw log
# Suppress noisy [N/M] Building/Scanning CXX ... progress lines
grep -E "warning:|error:|Error|FAILED|undefined reference|note:|fatal|Linking|Built target" "$RAW_LOG" >> "$LOG_FILE" 2>/dev/null || true

if [ $BUILD_EXIT -ne 0 ]; then
    {
        echo "========================================"
        echo " BUILD FAILED (exit code: $BUILD_EXIT)"
        echo " Finished: $(date)"
        echo "========================================"
    } >> "$LOG_FILE"

    echo "========================================"
    echo " ❌ BUILD FAILED!"
    echo " Check log: $LOG_FILE"
    echo " Full build output: $RAW_LOG"
    echo "========================================"
    exit $BUILD_EXIT
fi

# Clean up raw log on success
rm -f "$RAW_LOG"

{
    echo "========================================"
    echo " BUILD SUCCESSFUL"
    echo " Output: $BUILD_DIR/build/rme"
    echo " Finished: $(date)"
    echo "========================================"
} >> "$LOG_FILE"

echo "========================================"
echo " BUILD SUCCESSFUL!"
echo " Output: $BUILD_DIR/build/rme"
echo " Log: $LOG_FILE"
echo "========================================"
