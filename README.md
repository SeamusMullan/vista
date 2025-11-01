# vista

A fast, keyboard-driven wallpaper selector for Linux with smooth animations, grid view, favorites, and OpenGL shader support.

## Features

✅ **Thumbnail Caching** - MD5-based caching system for fast loading  
✅ **Smooth Animations** - Fluid scrolling transitions  
✅ **Grid View Mode** - Toggle between horizontal strip and grid layout  
✅ **Search & Filter** - Find wallpapers by filename  
✅ **Favorites System** - Mark and filter your favorite wallpapers  
✅ **OpenGL Shaders** - Hardware-accelerated rendering with custom effects  
✅ **Multiple Directories** - Scan wallpapers from multiple locations  
✅ **XDG Compliance** - Follows XDG base directory specification  
✅ **Mouse Support** - Click to select wallpapers  
✅ **Multiple Wallpaper Setters** - Support for feh, nitrogen, xwallpaper, swaybg  

## Keyboard Shortcuts

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

### Dependencies

- SDL2
- SDL2_image
- OpenSSL
- OpenGL and GLEW (optional, for shader support)

### Build

```bash
./build.sh
```

Or manually:

```bash
mkdir -p build
cd build
cmake ..
make
sudo make install
```

### Build with OpenGL Shaders

```bash
mkdir -p build
cd build
cmake -DUSE_SHADERS=ON ..
make
sudo make install
```

## Configuration

Vista follows XDG base directory specification. Configuration file location:
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

## License

See LICENSE file for details.
