# Building Vista

## Build System

Vista uses CMake and includes SDL3 and its companion libraries as git submodules. This means you can build the entire project without needing to install SDL3 system-wide.

## Prerequisites

- CMake 3.10 or later
- C compiler (GCC or Clang)
- OpenSSL development libraries
- Git

### Arch Linux
```bash
sudo pacman -S cmake gcc git openssl
```

### Ubuntu/Debian
```bash
sudo apt install cmake build-essential git libssl-dev
```

### macOS
```bash
brew install cmake openssl git
```

## Quick Start

1. **Clone the repository with submodules:**
```bash
git clone --recursive https://github.com/yourusername/vista.git
cd vista
```

If you already cloned without `--recursive`, initialize the submodules:
```bash
git submodule update --init --recursive
```

2. **Build the project:**
```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

The binary will be at `build/vista`.

## Build Options

### Use System SDL3 Libraries (Advanced)

If you have SDL3 installed system-wide and want to use it instead of building from source:

```bash
cmake -DUSE_SYSTEM_SDL=ON ..
make
```

### Enable OpenGL Shader Support

For experimental shader-based rendering:

```bash
cmake -DUSE_SHADERS=ON ..
make
```

Requires OpenGL and GLEW development libraries:
```bash
# Arch Linux
sudo pacman -S glew

# Ubuntu/Debian
sudo apt install libglew-dev

# macOS
brew install glew
```

### Release Build

For an optimized release build:

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

## Submodules

Vista includes the following libraries as submodules:

- **SDL3** - Core multimedia library
- **SDL3_image** - Image loading (PNG, JPG, BMP, etc.)
- **SDL3_mixer** - Audio mixing and playback
- **SDL3_ttf** - TrueType font rendering

These are automatically built when you build Vista, so you don't need to install them separately.

## Installation

```bash
sudo make install
```

This installs:
- Binary to `/usr/local/bin/vista`
- Example config to `/usr/local/share/vista/vista.conf.example`
- Shaders to `/usr/local/share/vista/shaders/`

## Troubleshooting

### Submodule errors

If you get errors about missing submodules:
```bash
git submodule update --init --recursive
```

### Build fails due to missing dependencies

Make sure you have OpenSSL development headers installed. On some systems you may also need:

```bash
# Arch Linux
sudo pacman -S base-devel

# Ubuntu/Debian
sudo apt install build-essential pkg-config
```

### Clean build

If you need to start fresh:
```bash
rm -rf build
mkdir build
cd build
cmake ..
make
```

## Development

### Generate compile_commands.json (for IDE support)

CMake automatically generates `compile_commands.json` in the build directory for use with language servers and IDEs.

### Build Documentation

If Doxygen is installed:
```bash
make docs
```

## Platform Notes

### Linux

- Works on X11 and Wayland
- Compositor support recommended for transparency effects
- Tested on Arch Linux

### macOS

- Requires macOS 10.14 or later
- Uses native file pickers and wallpaper setting

### Windows

- Build using Visual Studio or MinGW
- Some features may require Windows 10 or later
