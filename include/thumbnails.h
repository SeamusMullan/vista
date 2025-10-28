/**
 * @file thumbnails.h
 * @brief Thumbnail generation and caching
 */

#ifndef THUMBNAILS_H
#define THUMBNAILS_H

#include <SDL2/SDL.h>
#include "config.h"

/**
 * @brief Wallpaper structure
 */
typedef struct {
    char *path;           /**< Full path to wallpaper file */
    char *name;           /**< Filename without path */
    SDL_Surface *thumb;   /**< Loaded thumbnail surface */
} Wallpaper;

/**
 * @brief List of wallpapers
 */
typedef struct {
    Wallpaper *items;     /**< Array of wallpapers */
    int count;            /**< Number of wallpapers */
    int capacity;         /**< Allocated capacity */
} WallpaperList;

/**
 * @brief Scan directory for wallpapers
 * @param dir Directory path
 * @return List of found wallpapers
 */
WallpaperList wallpaper_list_scan(const char *dir);

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

#endif /* THUMBNAILS_H */
