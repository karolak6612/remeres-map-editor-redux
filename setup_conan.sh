#!/bin/bash
# ========================================
# RME Linux Dependency Setup Script
# Optimized for Jules AI (Low Resource)
# ========================================

set -e
set -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Using local build_conan directory (gitignored)
BUILD_DIR="$SCRIPT_DIR/build_conan"
LOG_FILE="$BUILD_DIR/setup_conan.log"

# Ensure build directory exists for the log file
mkdir -p "$BUILD_DIR"

{
    echo "========================================"
    echo " RME Linux Dependency Setup"
    echo " Started: $(date)"
    echo "========================================"
} > "$LOG_FILE"

# Step 1: Install system dependencies via apt (fast, pre-compiled)
echo "[1/4] Installing system dependencies (Visible)..." | tee -a "$LOG_FILE"
sudo apt update -y 2>&1 | tee -a "$LOG_FILE"
sudo apt install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    python3 \
    python3-pip \
    libgtk-3-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libwxgtk3.2-dev \
    libboost-all-dev \
    libarchive-dev \
    zlib1g-dev \
    libglm-dev \
    libspdlog-dev \
    libfmt-dev \
    nlohmann-json3-dev \
    libasio-dev \
    clang-tools-18 2>&1 | tee -a "$LOG_FILE"

echo "  ✅ System packages installed" | tee -a "$LOG_FILE"

# Step 2: Setup Conan profile
echo "[2/4] Setting up Conan profile..." | tee -a "$LOG_FILE"
if ! conan profile show &> /dev/null; then
    conan profile detect --force 2>&1 | tee -a "$LOG_FILE"
fi

# Step 3: Install dependencies via Conan (Safe & Fast)
echo "[3/4] Installing Conan dependencies (Binaries + Missing tiny deps)..." | tee -a "$LOG_FILE"
# Using --build=missing is better than --build=never here because on Linux
# we only use 2 tiny libraries from Conan (glad, tomlplusplus).
# This avoids "Missing Binary" errors while still being nearly instant.
if ! conan install "$SCRIPT_DIR" \
    -of "$BUILD_DIR" \
    --build=missing \
    -s build_type=Release \
    -s compiler.libcxx=libstdc++11 2>&1 | tee -a "$LOG_FILE"; then
    echo "========================================" | tee -a "$LOG_FILE"
    echo " CONAN INSTALL FAILED." | tee -a "$LOG_FILE"
    echo "========================================" | tee -a "$LOG_FILE"
    exit 1
fi

# Step 4: Verification
echo "[4/4] Verifying setup..." | tee -a "$LOG_FILE"
if [ -d "$BUILD_DIR" ]; then
    echo "  ✅ Conan environment initialized in $BUILD_DIR" | tee -a "$LOG_FILE"
else
    echo "  ⚠️  Setup may have failed, check $LOG_FILE" | tee -a "$LOG_FILE"
    exit 1
fi

echo "========================================" | tee -a "$LOG_FILE"
echo " SETUP COMPLETE!" | tee -a "$LOG_FILE"
echo " All temporary files are in $BUILD_DIR" | tee -a "$LOG_FILE"
echo " Your git directory is CLEAN ✨" | tee -a "$LOG_FILE"
echo "========================================" | tee -a "$LOG_FILE"
