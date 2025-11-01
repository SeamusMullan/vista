/**
 * @file renderer.h
 * @brief SDL2 rendering functions
 */

#ifndef RENDERER_H
#define RENDERER_H

#include <SDL2/SDL.h>
#include "config.h"
#include "thumbnails.h"

/**
 * @brief View mode enumeration
 */
typedef enum {
    VIEW_MODE_HORIZONTAL,     /**< Horizontal scrolling view */
    VIEW_MODE_GRID           /**< Grid layout view */
} ViewMode;

/**
 * @brief Renderer state
 */
typedef struct {
    SDL_Window *window;       /**< SDL window */
    SDL_Renderer *renderer;   /**< SDL renderer */
    int selected_index;       /**< Currently selected wallpaper */
    int scroll_offset;        /**< Horizontal scroll offset */
    float target_scroll;      /**< Target scroll position for smooth animation */
    float current_scroll;     /**< Current scroll position for smooth animation */
    ViewMode view_mode;       /**< Current view mode */
    int grid_scroll_y;        /**< Vertical scroll for grid mode */
    float target_scroll_y;    /**< Target vertical scroll */
    float current_scroll_y;   /**< Current vertical scroll */
    bool search_mode;         /**< Whether in search mode */
    bool show_help;           /**< Whether to show help overlay */
} Renderer;

/**
 * @brief Initialize renderer
 * @param config Configuration
 * @return Initialized renderer
 */
Renderer* renderer_init(const Config *config);

/**
 * @brief Render current frame
 * @param r Renderer state
 * @param list Wallpaper list
 * @param config Configuration
 */
void renderer_draw_frame(Renderer *r, const WallpaperList *list, const Config *config);

/**
 * @brief Move selection left
 * @param r Renderer state
 * @param config Configuration
 */
void renderer_select_prev(Renderer *r, const Config *config);

/**
 * @brief Move selection right
 * @param r Renderer state
 * @param max Maximum index
 * @param config Configuration
 */
void renderer_select_next(Renderer *r, int max, const Config *config);

/**
 * @brief Move selection up (grid mode)
 * @param r Renderer state
 * @param config Configuration
 */
void renderer_select_up(Renderer *r, const Config *config);

/**
 * @brief Move selection down (grid mode)
 * @param r Renderer state
 * @param max Maximum index
 * @param config Configuration
 */
void renderer_select_down(Renderer *r, int max, const Config *config);

/**
 * @brief Toggle between horizontal and grid view modes
 * @param r Renderer state
 */
void renderer_toggle_view_mode(Renderer *r);

/**
 * @brief Draw help overlay
 * @param r Renderer state
 */
void renderer_draw_help_overlay(Renderer *r);

/**
 * @brief Cleanup renderer resources
 * @param r Renderer state
 */
void renderer_cleanup(Renderer *r);

#endif /* RENDERER_H */
