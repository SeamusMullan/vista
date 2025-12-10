/**
 * @file openrgb.h
 * @brief OpenRGB CLI integration for peripheral color control
 */

#ifndef OPENRGB_H
#define OPENRGB_H

#include <stdbool.h>
#include "color_source.h"
#include "config.h"

/**
 * @brief Check if OpenRGB CLI is available
 * @return true if openrgb command is in PATH, false otherwise
 */
bool openrgb_is_available(void);

/**
 * @brief Set all OpenRGB devices to a single color using CLI
 * @param color RGB color to set
 * @param mode OpenRGB mode (e.g., "static", "breathing", "spectrum_cycle")
 * @return 0 on success, -1 on error
 */
int openrgb_set_color_cli(RGBColor color, const char *mode);

/**
 * @brief Set all OpenRGB devices to a single color with brightness
 * @param color RGB color to set
 * @param mode OpenRGB mode
 * @param brightness Brightness percentage (0-100), or -1 to ignore
 * @return 0 on success, -1 on error
 */
int openrgb_set_color_cli_brightness(RGBColor color, const char *mode, int brightness);

/**
 * @brief Apply color from configured color source to OpenRGB
 * @param wallpaper_path Path to current wallpaper
 * @param config Configuration
 * @return 0 on success, -1 on error
 */
int openrgb_apply_from_config(const char *wallpaper_path, const Config *config);

#endif /* OPENRGB_H */
