/**
 * @file color_source.h
 * @brief Color extraction from various sources for OpenRGB integration
 */

#ifndef COLOR_SOURCE_H
#define COLOR_SOURCE_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

/**
 * @brief RGB color structure
 */
typedef struct {
    uint8_t r;  /**< Red component (0-255) */
    uint8_t g;  /**< Green component (0-255) */
    uint8_t b;  /**< Blue component (0-255) */
} RGBColor;

/**
 * @brief Color palette structure
 */
typedef struct {
    RGBColor colors[16];  /**< Array of colors */
    int count;            /**< Number of colors in palette */
} ColorPalette;

/**
 * @brief Color source types
 */
typedef enum {
    COLOR_SOURCE_WAL,      /**< Read from pywal cache (~/.cache/wal/colors) */
    COLOR_SOURCE_SCRIPT,   /**< Run custom script that outputs hex color */
    COLOR_SOURCE_STATIC,   /**< Use static configured color */
} ColorSourceType;

/**
 * @brief Parse color source type from string
 * @param source_str String representation ("wal", "script", "static")
 * @return ColorSourceType enum value (defaults to WAL if invalid)
 */
ColorSourceType color_source_parse_type(const char *source_str);

/**
 * @brief Get primary color from configured source
 * @param source Color source type
 * @param wallpaper_path Path to wallpaper (used by script source)
 * @param config Configuration
 * @return RGB color (returns white on error)
 */
RGBColor color_source_get_primary(ColorSourceType source, const char *wallpaper_path, const Config *config);

/**
 * @brief Get color palette from source
 * @param source Color source type
 * @param wallpaper_path Path to wallpaper (used by script source)
 * @param config Configuration
 * @return Color palette
 */
ColorPalette color_source_get_palette(ColorSourceType source, const char *wallpaper_path, const Config *config);

/**
 * @brief Read primary color from pywal cache
 * @return RGB color (returns white if cache not found)
 */
RGBColor color_source_read_wal(void);

/**
 * @brief Read color palette from pywal cache
 * @return Color palette
 */
ColorPalette color_source_read_wal_palette(void);

/**
 * @brief Convert hex string to RGB color
 * @param hex Hex color string (formats: "RRGGBB", "#RRGGBB", "0xRRGGBB")
 * @return RGB color
 */
RGBColor color_hex_to_rgb(const char *hex);

/**
 * @brief Convert RGB color to hex string
 * @param color RGB color
 * @param buffer Output buffer (must be at least 7 bytes)
 */
void color_rgb_to_hex(RGBColor color, char *buffer);

#endif /* COLOR_SOURCE_H */
