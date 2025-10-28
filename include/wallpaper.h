/**
 * @file wallpaper.h
 * @brief Wallpaper application logic
 */

#ifndef WALLPAPER_H
#define WALLPAPER_H

#include "config.h"

/**
 * @brief Apply wallpaper using feh
 * @param path Path to wallpaper file
 * @param config Configuration
 * @return 0 on success, -1 on error
 */
int wallpaper_apply(const char *path, const Config *config);

/**
 * @brief Run palette generation script
 * @param wallpaper_path Path to wallpaper
 * @param config Configuration
 * @return 0 on success, -1 on error
 */
int wallpaper_generate_palette(const char *wallpaper_path, const Config *config);

#endif /* WALLPAPER_H */
