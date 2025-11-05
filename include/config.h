/**
 * @file config.h
 * @brief Configuration parsing and management
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

#define MAX_PATH 256

#define MAX_WALLPAPER_DIRS 10
#define MAX_MONITORS 8
#define MAX_COMMAND 512

/**
 * @brief Configuration structure
 */
typedef struct {
    char wallpaper_dir[MAX_PATH];              /**< Primary directory containing wallpapers */
    char wallpaper_dirs[MAX_WALLPAPER_DIRS][MAX_PATH]; /**< Multiple wallpaper directories */
    int wallpaper_dirs_count;                  /**< Number of configured directories */
    char feh_command[MAX_PATH];                /**< Command to set wallpaper */
    char palette_script[MAX_PATH];             /**< Script to generate color palette */
    
    // Multi-monitor support
    char monitors[MAX_MONITORS][64];           /**< Monitor output names (e.g., "DP-2", "HDMI-1") */
    int monitors_count;                        /**< Number of configured monitors */
    bool use_per_monitor;                      /**< Apply wallpaper per monitor instead of spanning */
    
    // Additional commands
    bool use_wal;                              /**< Generate colors using pywal */
    char wal_options[256];                     /**< Additional options to pass to wal command */
    bool reload_i3;                            /**< Reload i3 after wallpaper change */
    char post_command[MAX_COMMAND];            /**< Additional command to run after wallpaper change */
    
    int thumbnail_width;                       /**< Thumbnail width in pixels */
    int thumbnail_height;                      /**< Thumbnail height in pixels */
    int window_width;                          /**< Window width */
    int window_height;                         /**< Window height */
    bool use_shaders;                          /**< Enable shader rendering */
    int thumbnails_per_row;                    /**< Number of thumbnails per row */
    
    char audio_dir[MAX_PATH];                  /**< Directory containing audio files for roulette */
    
    // Roulette animation timing (in milliseconds)
    int roulette_start_duration;               /**< Acceleration phase duration */
    int roulette_scroll_duration;              /**< Fast scrolling phase duration */
    int roulette_slow_duration;                /**< Deceleration phase duration */
    int roulette_show_duration;                /**< Final display duration */
    float roulette_max_velocity;               /**< Maximum scroll velocity (items/sec) */

    const char *file_location;                   /**< Location of config file on disk */
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
