#!/bin/bash

# Check if wallpaper path is provided
if [ -z "$1" ]; then
    echo "Usage: $0 /path/to/wallpaper.jpg"
    WALLPAPER="$HOME/Pictures/wallpaper.jpg"  # Change this path
    if [ ! -f "$WALLPAPER" ]; then
        echo "No wallpaper specified and default not found"
        exit 1
    fi
else
    WALLPAPER="$1"
fi

# Generate colors from wallpaper
wal -i "$WALLPAPER"

# Set wallpaper per monitor instead of stretching across both
feh --bg-fill --output DP-2 "$WALLPAPER" --bg-fill --output HDMI-1 "$WALLPAPER"

# Alternative: Different wallpapers per monitor
# feh --bg-scale --output DP-2 "$WALLPAPER" --bg-scale --output HDMI-1 "/path/to/vertical-wallpaper.jpg"

# Reload i3 configuration
i3-msg reload

# Restart polybar
#killall -q polybar
#while pgrep -u $UID -x polybar >/dev/null; do sleep 1; done
#~/.config/polybar/launch.sh

echo "Theme updated successfully!"
