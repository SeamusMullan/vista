#!/bin/bash
# Quick setup script for vista with wallpaper-switcher integration

CONFIG_DIR="$HOME/.config/vista"
CONFIG_FILE="$CONFIG_DIR/vista.conf"

echo "Setting up vista configuration..."

# Create config directory
mkdir -p "$CONFIG_DIR"

# Copy configuration
cp "$(dirname "$0")/config/vista.conf" "$CONFIG_FILE"

echo "✓ Configuration installed to: $CONFIG_FILE"
echo ""
echo "Configuration details:"
echo "  - Wallpaper directory: ~/wallpaper/desktopGenerations"
echo "  - Theme script: ~/.local/bin/theme-update.sh"
echo "  - Wallpaper setter: feh --bg-fill"
echo ""
echo "To customize the configuration, edit: $CONFIG_FILE"
echo ""
echo "Launch vista with: ./build/vista"
