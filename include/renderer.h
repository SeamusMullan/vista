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
 * @brief Renderer state
 */
typedef struct {
    SDL_Window *window;       /**< SDL window */
    SDL_Renderer *renderer;   /**< SDL renderer */
    int selected_index;       /**< Currently selected wallpaper */
    int scroll_offset;        /**< Horizontal scroll offset */
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
 */
void renderer_select_prev(Renderer *r);

/**
 * @brief Move selection right
 * @param r Renderer state
 * @param max Maximum index
 */
void renderer_select_next(Renderer *r, int max);

/**
 * @brief Clean up renderer
 * @param r Renderer to free
 */
void renderer_cleanup(Renderer *r);

#endif /* RENDERER_H */
