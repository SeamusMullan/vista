/**
 * @file thumbnails.h
 * @brief Thumbnail generation and caching
 */

#ifndef THUMBNAILS_H
#define THUMBNAILS_H

#include <SDL3/SDL.h>
#include "config.h"

/**
 * @brief Wallpaper structure
 */
typedef struct {
    char *path;           /**< Full path to wallpaper file */
    char *name;           /**< Filename without path */
    SDL_Surface *thumb;   /**< Loaded thumbnail surface */
    bool is_favorite;     /**< Whether wallpaper is marked as favorite */
} Wallpaper;

/**
 * @brief List of wallpapers
 */
typedef struct {
    Wallpaper *items;     /**< Array of wallpapers */
    int count;            /**< Number of wallpapers */
    int capacity;         /**< Allocated capacity */
    char search_query[256]; /**< Current search query */
    int *filtered_indices; /**< Indices of filtered wallpapers */
    int filtered_count;    /**< Number of filtered wallpapers */
    bool show_favorites_only; /**< Filter to show only favorites */
} WallpaperList;

/**
 * @brief Scan directory for wallpapers
 * @param dir Directory path
 * @return List of found wallpapers
 */
WallpaperList wallpaper_list_scan(const char *dir);

/**
 * @brief Scan multiple directories for wallpapers
 * @param config Configuration containing directory list
 * @return List of found wallpapers
 */
WallpaperList wallpaper_list_scan_multiple(const Config *config);

/**
 * @brief Generate thumbnails for all wallpapers
 * @param list Wallpaper list
 * @param config Configuration
 */
void wallpaper_list_generate_thumbnails(WallpaperList *list, const Config *config);

/**
 * @brief Load or create cached thumbnail
 * @param path Original image path
 * @param width Thumbnail width
 * @param height Thumbnail height
 * @return Loaded thumbnail surface
 */
SDL_Surface* thumbnail_load_or_cache(const char *path, int width, int height);

/**
 * @brief Free wallpaper list
 * @param list List to free
 */
void wallpaper_list_free(WallpaperList *list);

/**
 * @brief Apply search filter to wallpaper list
 * @param list Wallpaper list
 * @param query Search query
 */
void wallpaper_list_filter(WallpaperList *list, const char *query);

/**
 * @brief Clear search filter
 * @param list Wallpaper list
 */
void wallpaper_list_clear_filter(WallpaperList *list);

/**
 * @brief Get wallpaper at display index (accounts for filtering)
 * @param list Wallpaper list
 * @param index Display index
 * @return Wallpaper pointer or NULL
 */
Wallpaper* wallpaper_list_get(WallpaperList *list, int index);

/**
 * @brief Get visible count (accounts for filtering)
 * @param list Wallpaper list
 * @return Number of visible wallpapers
 */
int wallpaper_list_visible_count(const WallpaperList *list);

/**
 * @brief Toggle favorite status of wallpaper
 * @param list Wallpaper list
 * @param index Display index
 */
void wallpaper_toggle_favorite(WallpaperList *list, int index);

/**
 * @brief Toggle favorites-only filter
 * @param list Wallpaper list
 */
void wallpaper_list_toggle_favorites_filter(WallpaperList *list);

/**
 * @brief Load favorites from disk
 * @param list Wallpaper list
 */
void wallpaper_list_load_favorites(WallpaperList *list);

/**
 * @brief Save favorites to disk
 * @param list Wallpaper list
 */
void wallpaper_list_save_favorites(const WallpaperList *list);

#endif /* THUMBNAILS_H */
