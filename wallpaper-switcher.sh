#!/bin/bash

# CONFIGURATION
WALLPAPER_DIR="$HOME/wallpaper/desktopGenerations"
THEME_SCRIPT="$HOME/.local/bin/theme-update.sh"
STATE_FILE="$HOME/.cache/current_wallpaper_index"

# Get list of wallpapers recursively from all subfolders
mapfile -t WALLPAPERS < <(find "$WALLPAPER_DIR" -type f \( -iname '*.png' -o -iname '*.jpg' -o -iname '*.jpeg' \) | sort)

TOTAL=${#WALLPAPERS[@]}
if [ "$TOTAL" -eq 0 ]; then
    echo "No wallpapers found in $WALLPAPER_DIR"
    exit 1
fi

# Determine current index
if [ -f "$STATE_FILE" ]; then
    INDEX=$(<"$STATE_FILE")
else
    INDEX=0
fi

# Get command-line argument (next, prev, random, list, <number>)
case "$1" in
    next)
        INDEX=$(( (INDEX + 1) % TOTAL ))
        ;;
    prev)
        INDEX=$(( (INDEX - 1 + TOTAL) % TOTAL ))
        ;;
    random)
        INDEX=$(( RANDOM % TOTAL ))
        ;;
    list)
        for i in "${!WALLPAPERS[@]}"; do
            echo "[$i] ${WALLPAPERS[$i]}"
        done
        exit 0
        ;;
    [0-9]*)
        if [ "$1" -ge 0 ] && [ "$1" -lt "$TOTAL" ]; then
            INDEX="$1"
        else
            echo "Error: Index must be between 0 and $((TOTAL - 1))"
            exit 1
        fi
        ;;
    *)
        echo "Usage: $0 [next|prev|random|list|<index>]"
        exit 1
        ;;
esac

# Save new index
echo "$INDEX" > "$STATE_FILE"

# Set wallpaper
WALLPAPER="${WALLPAPERS[$INDEX]}"
feh --bg-fill "$WALLPAPER"
"$THEME_SCRIPT" "$WALLPAPER"

