/**
 * @file config.h
 * @brief Configuration parsing and management
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

#define MAX_PATH 256

/**
 * @brief Configuration structure
 */
typedef struct {
    char wallpaper_dir[MAX_PATH];    /**< Directory containing wallpapers */
    char feh_command[MAX_PATH];      /**< Command to set wallpaper */
    char palette_script[MAX_PATH];   /**< Script to generate color palette */
    int thumbnail_width;              /**< Thumbnail width in pixels */
    int thumbnail_height;             /**< Thumbnail height in pixels */
    int window_width;                 /**< Window width */
    int window_height;                /**< Window height */
    bool use_shaders;                 /**< Enable shader rendering */
    int thumbnails_per_row;           /**< Number of thumbnails per row */
} Config;

/**
 * @brief Parse configuration file
 * @param path Path to config file
 * @return Parsed configuration
 */
Config config_parse(const char *path);

/**
 * @brief Get default configuration
 * @return Default configuration values
 */
Config config_default(void);

/**
 * @brief Print configuration (for debugging)
 * @param config Configuration to print
 */
void config_print(const Config *config);

#endif /* CONFIG_H */
