#!/bin/bash
# Build script for vista

set -e

BUILD_TYPE="${1:-Release}"
USE_SHADERS="${2:-OFF}"

echo "Building vista..."
echo "  Build type: $BUILD_TYPE"
echo "  Shaders: $USE_SHADERS"

# remove existing build directory if exists
rm -rf build

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DUSE_SHADERS="$USE_SHADERS" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build
cmake --build . -j$(nproc)

echo ""
echo "Build complete! Binary: ./build/vista"
echo ""
echo "To run from build directory:"
echo "  ./build/vista"
