#!/bin/bash
# Build script for vista

set -e

# Default values
BUILD_TYPE="Debug"
USE_SHADERS="OFF"
RECONFIGURE=false

# Parse command line arguments
while getopts "rshc" opt; do
    case $opt in
        r)
            BUILD_TYPE="Release"
            ;;
        s)
            USE_SHADERS="ON"
            ;;
        c)
            RECONFIGURE=true
            ;;
        h)
            echo "Usage: $0 [-r] [-s] [-c] [-h]"
            echo ""
            echo "Build script for vista"
            echo ""
            echo "Options:"
            echo "  -r    Build in Release mode (default: Debug)"
            echo "  -s    Enable shader support (default: OFF)"
            echo "  -c    Reconfigure CMake (remove and recreate build directory)"
            echo "  -h    Show this help message"
            exit 0
            ;;
        \?)
            echo "Usage: $0 [-r] [-s] [-c] [-h]"
            echo "Use -h for more information"
            exit 1
            ;;
    esac
done

echo "Building vista..."
echo "  Build type: $BUILD_TYPE"
echo "  Shaders: $USE_SHADERS"

# Handle CMake configuration
if [ "$RECONFIGURE" = true ]; then
    echo "  Reconfiguring: YES"
    # Remove existing build directory if exists
    rm -rf build
    # Create build directory
    mkdir -p build
    cd build
    # Configure with CMake
    cmake .. \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DUSE_SHADERS="$USE_SHADERS" \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
else
    echo "  Reconfiguring: NO"
    # Assume build directory exists
    if [ ! -d "build" ]; then
        echo "Error: build directory does not exist. Run with -c to configure."
        exit 1
    fi
    cd build
fi

# Build using all available cores
cmake --build . -j$(nproc --all)

echo ""
echo "Build complete! Binary: ./build/vista"
echo ""

# Determine shader installation path based on platform
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS: use /usr/local to avoid SIP restrictions
    SHADER_DIR="/usr/local/share/vista/shaders"
else
    # Linux and others: use /usr/share
    SHADER_DIR="/usr/share/vista/shaders"
fi

if [ "$USE_SHADERS" = "ON" ]; then
    echo "Installing shaders to $SHADER_DIR..."
    sudo mkdir -p "$SHADER_DIR"
    sudo cp ../shaders/vertex.glsl "$SHADER_DIR/vertex.glsl"
    sudo cp ../shaders/fragment.glsl "$SHADER_DIR/fragment.glsl"
    echo "Shaders installed successfully!"
fi

echo "To run from build directory:"
echo "  ./build/vista"
