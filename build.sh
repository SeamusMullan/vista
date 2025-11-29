#!/bin/bash
# Build script for vista

set -e

# Default values
BUILD_TYPE="Debug"
USE_SHADERS="OFF"

# Parse command line arguments
while getopts "rsh" opt; do
    case $opt in
        r)
            BUILD_TYPE="Release"
            ;;
        s)
            USE_SHADERS="ON"
            ;;
        h)
            echo "Usage: $0 [-r] [-s] [-h]"
            echo ""
            echo "Build script for vista"
            echo ""
            echo "Options:"
            echo "  -r    Build in Release mode (default: Debug)"
            echo "  -s    Enable shader support (default: OFF)"
            echo "  -h    Show this help message"
            exit 0
            ;;
        \?)
            echo "Usage: $0 [-r] [-s] [-h]"
            echo "Use -h for more information"
            exit 1
            ;;
    esac
done

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

# Build using all available cores
cmake --build . -j$(nproc --all)

echo ""
echo "Build complete! Binary: ./build/vista"
echo ""
echo "To run from build directory:"
echo "  ./build/vista"
