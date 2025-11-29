# vista

A fast, keyboard-driven wallpaper switcher for Linux with smooth animations, favorites, and OpenGL shader support.


https://github.com/user-attachments/assets/0874e86c-f6e2-4d63-ac55-0971fc397c5c

> song is No Mana- Lost Call, wallpapers from desktopgeneration.com

## Features

**Thumbnail Caching** - MD5-based caching system for fast loading  
**Smooth Animations** - Fluid scrolling transitions  
**Grid View Mode** - Toggle between horizontal strip and grid layout  
**Search & Filter** - Find wallpapers by filename  
**Favorites System** - Mark and filter your favorite wallpapers  
**OpenGL Shaders** - Hardware-accelerated rendering with custom effects  
**Multiple Directories** - Scan wallpapers from multiple locations  
**Multiple Wallpaper Setters** - Support for feh, nitrogen, xwallpaper, swaybg  

## Keyboard Controls

| Key | Action |
|-----|--------|
| `←/→` or `h/l` | Navigate left/right |
| `↑/↓` or `k/j` | Navigate up/down (grid mode) |
| `Enter` | Apply selected wallpaper |
| `g` | Toggle grid/horizontal view |
| `f` | Toggle favorite on current wallpaper |
| `F2` | Filter to show only favorites |
| `/` or `?` | Toggle help overlay |
| `q` or `Esc` | Quit |

## Installation

### Quick Build (Recommended)

Vista includes SDL3 and its companion libraries as git submodules, so you don't need to install them separately!

```bash
git clone --recursive https://github.com/yourusername/vista.git
cd vista
mkdir build
cd build
cmake ..
make -j$(nproc)
```

The binary will be at `build/vista`.

**See [BUILD.md](BUILD.md) for detailed build instructions and options.**

### Dependencies

Only minimal dependencies are required:

- **CMake** 3.10 or later
- **C compiler** (GCC or Clang)
- **OpenSSL** development libraries
- **Git**

SDL3, SDL3_image, SDL3_mixer, and SDL3_ttf are included as submodules and built automatically.

#### Installing dependencies:

**Arch Linux:**
```bash
sudo pacman -S cmake gcc git openssl
```

**Ubuntu/Debian:**
```bash
sudo apt install cmake build-essential git libssl-dev
```

**macOS:**
```bash
brew install cmake openssl git
```

### Optional: OpenGL Shader Support

For experimental hardware-accelerated rendering:

```bash
cmake -DUSE_SHADERS=ON ..
make
```

Requires: OpenGL and GLEW development libraries

### Installation

```bash
sudo make install
```

## Configuration

Configuration file location:
- `$XDG_CONFIG_HOME/vista/vista.conf`
- or `~/.config/vista/vista.conf`

### Example Configuration

```ini
# Primary wallpaper directory
wallpaper_dir = /home/user/wallpapers

# Additional wallpaper directories
wallpaper_dir_1 = /home/user/Pictures/Wallpapers
wallpaper_dir_2 = /usr/share/backgrounds

# Wallpaper setter command
# Options: feh, nitrogen, xwallpaper, swaybg, or custom
feh_command = feh --bg-scale

# Thumbnail dimensions
thumbnail_width = 200
thumbnail_height = 150

# Window dimensions
window_width = 1200
window_height = 300

# Grid view settings
thumbnails_per_row = 5

# Enable OpenGL shader rendering
use_shaders = false

# Optional: palette generation script
palette_script = /path/to/palette-generator.sh
```

## Cache and Data Locations

Following XDG specification:

- **Thumbnails**: `$XDG_CACHE_HOME/vista/` or `~/.cache/vista/`
- **Favorites**: `$XDG_DATA_HOME/vista/favorites.txt` or `~/.local/share/vista/favorites.txt`

## Wallpaper Setters

Vista supports multiple wallpaper setting tools:

- **feh**: `feh_command = feh --bg-scale`
- **nitrogen**: `feh_command = nitrogen`
- **xwallpaper**: `feh_command = xwallpaper`
- **swaybg** (Wayland): `feh_command = swaybg`
- **Custom**: Use any command with `%s` as placeholder for image path

## Usage Examples

```bash
# Use default config
vista

# Use custom config
vista -c /path/to/config

# Show help
vista --help

# Show version
vista --version
```

