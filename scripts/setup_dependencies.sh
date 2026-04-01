#!/bin/bash
# Gini Engine - Dependency Setup Script
# This script downloads and sets up third-party dependencies

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
THIRDPARTY_DIR="$PROJECT_ROOT/ThirdParty"

echo "=== Gini Engine Dependency Setup ==="
echo "Project Root: $PROJECT_ROOT"

# Create directories
mkdir -p "$THIRDPARTY_DIR/glad/include/glad"
mkdir -p "$THIRDPARTY_DIR/glad/include/KHR"
mkdir -p "$THIRDPARTY_DIR/glad/src"
mkdir -p "$THIRDPARTY_DIR/stb"

# Download GLAD (OpenGL 4.6 Core Profile)
echo "Downloading GLAD..."
if command -v python3 &> /dev/null; then
    pip3 install glad --quiet 2>/dev/null || true
    python3 -m glad --profile core --api gl=4.6 --generator c --out-path "$THIRDPARTY_DIR/glad" --reproducible
else
    echo "Python3 not found. Please install GLAD manually or install Python3."
    echo "You can download GLAD from: https://glad.dav1d.de/"
    echo "Settings: Language=C/C++, Specification=OpenGL, Profile=Core, API gl=4.6"
fi

echo "=== Setup Complete ==="
echo "Run 'cmake -B build && cmake --build build' to build the engine."
