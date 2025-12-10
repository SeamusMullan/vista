#!/bin/bash
# Build script for vista

set -e

# Default values
BUILD_TYPE="Debug"
USE_SHADERS="OFF"
RECONFIGURE=false
INSTALL=false
INSTALL_PREFIX="/usr/local"

# Parse command line arguments
while getopts "rshcip:" opt; do
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
        i)
            INSTALL=true
            ;;
        p)
            INSTALL_PREFIX="$OPTARG"
            ;;
        h)
            echo "Usage: $0 [-r] [-s] [-c] [-i] [-p PREFIX] [-h]"
            echo ""
            echo "Build script for vista"
            echo ""
            echo "Options:"
            echo "  -r    Build in Release mode (default: Debug)"
            echo "  -s    Enable shader support (default: OFF)"
            echo "  -c    Reconfigure CMake (remove and recreate build directory)"
            echo "  -i    Install after building (requires sudo)"
            echo "  -p    Installation prefix (default: /usr/local, use \$HOME/.local for user install)"
            echo "  -h    Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0 -r -s -c -i              # Build release with shaders and install to /usr/local"
            echo "  $0 -r -s -i -p \$HOME/.local  # Build and install to ~/.local (no sudo needed)"
            exit 0
            ;;
        \?)
            echo "Usage: $0 [-r] [-s] [-c] [-i] [-p PREFIX] [-h]"
            echo "Use -h for more information"
            exit 1
            ;;
    esac
done

echo "Building vista..."
echo "  Build type: $BUILD_TYPE"
echo "  Shaders: $USE_SHADERS"
if [ "$INSTALL" = true ]; then
    echo "  Install: YES (to $INSTALL_PREFIX)"
else
    echo "  Install: NO"
fi

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

if [ "$INSTALL" = true ]; then
echo "Installing vista to $INSTALL_PREFIX..."

    # Check if we need sudo
    if [ "$INSTALL_PREFIX" = "/usr/local" ] || [ "$INSTALL_PREFIX" = "/usr" ]; then
        SUDO_CMD="sudo"
    else
        SUDO_CMD=""
    fi

    # Configure with install prefix
    $SUDO_CMD cmake -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" ..

    # Run make install
    $SUDO_CMD make install

    if [ "$USE_SHADERS" = "ON" ]; then

        # Determine shader installation path based on platform
        if [[ "$OSTYPE" == "darwin"* ]]; then
            # macOS: use /usr/local to avoid SIP restrictions
            SHADER_DIR="/usr/local/share/vista/shaders"
        else
            # Linux and others: use /usr/share
            SHADER_DIR="/usr/share/vista/shaders"
        fi

        echo "Installing shaders to $SHADER_DIR..."
        sudo mkdir -p "$SHADER_DIR"
        sudo cp ../shaders/vertex.glsl "$SHADER_DIR/vertex.glsl"
        sudo cp ../shaders/fragment.glsl "$SHADER_DIR/fragment.glsl"
        echo "Shaders installed successfully!"
    fi

    echo ""
    echo "Installation complete!"
    echo "  Binary: $INSTALL_PREFIX/bin/vista"
    echo "  Libraries: $INSTALL_PREFIX/lib/"
    echo "  Shaders: $INSTALL_PREFIX/share/vista/shaders/"
    echo ""
    if [ "$INSTALL_PREFIX" = "$HOME/.local" ]; then
        echo "Make sure $HOME/.local/bin is in your PATH:"
        echo "  export PATH=\"\$HOME/.local/bin:\$PATH\""
    fi
else
    echo "To run from build directory:"
    echo "  ./build/vista"
    echo ""
    echo "To install, run:"
    echo "  ./build.sh -i              # Install to /usr/local (requires sudo)"
    echo "  ./build.sh -i -p \$HOME/.local  # Install to ~/.local (no sudo)"
fi



