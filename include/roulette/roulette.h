/**
 * @file roulette.h
 * @brief CS:GO-style case opening animation for random wallpaper selection
 */

#ifndef ROULETTE_H
#define ROULETTE_H

#include <SDL2/SDL.h>
#ifdef HAVE_SDL_MIXER
#include <SDL2/SDL_mixer.h>
#endif
#include "../thumbnails.h"
#include "../config.h"

/**
 * @brief Animation state for the roulette
 */
typedef enum {
    ROULETTE_STATE_STARTING,    /**< Initial acceleration phase */
    ROULETTE_STATE_SCROLLING,   /**< Fast scrolling phase */
    ROULETTE_STATE_SLOWING,     /**< Deceleration to exact target */
    ROULETTE_STATE_SELECTING,   /**< Unused - kept for compatibility */
    ROULETTE_STATE_SHOWING,     /**< Display selected wallpaper */
    ROULETTE_STATE_FINISHED     /**< Animation complete */
} RouletteState;

/**
 * @brief Roulette animation context
 */
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    RouletteState state;
    
    float scroll_position;      /**< Current scroll position */
    float scroll_velocity;      /**< Current velocity */
    float target_position;      /**< Target position for selected item */
    float selecting_start_pos;  /**< Position when entering selecting state */
    int selected_index;         /**< Index of the selected wallpaper */
    
    Uint32 state_start_time;    /**< When current state started */
    Uint32 animation_start_time; /**< When animation started */
    
    // Animation timing parameters
    float start_duration;       /**< Duration of starting phase (ms) */
    float scroll_duration;      /**< Duration of fast scroll phase (ms) */
    float slow_duration;        /**< Duration of slowing to target (ms) */
    float select_duration;      /**< Unused - kept for compatibility */
    float show_duration;        /**< Duration to show result (ms) */
    
    float max_velocity;         /**< Maximum scroll velocity */
    
    // Visual parameters
    int item_width;             /**< Width of each item in carousel */
    int item_height;            /**< Height of each item in carousel */
    int item_spacing;           /**< Spacing between items */
    int center_x;               /**< Center X position on screen */
    int center_y;               /**< Center Y position on screen */
    
    // Randomization
    int loops;                  /**< Number of full loops before stopping */
    
    // Audio
#ifdef HAVE_SDL_MIXER
    Mix_Chunk *tick_sound;      /**< Sound when passing an item */
    Mix_Chunk *select_sound;    /**< Sound for final selection */
#endif
    int last_item_index;        /**< Track last item for tick sound */
} RouletteContext;

/**
 * @brief Initialize roulette animation
 * @param config Application configuration
 * @param wallpapers List of available wallpapers
 * @return Initialized roulette context or NULL on error
 */
RouletteContext* roulette_init(const Config *config, WallpaperList *wallpapers);

/**
 * @brief Run the roulette animation
 * @param ctx Roulette context
 * @param wallpapers List of wallpapers
 * @return Index of selected wallpaper
 */
int roulette_run(RouletteContext *ctx, WallpaperList *wallpapers);

/**
 * @brief Update roulette animation state
 * @param ctx Roulette context
 * @param wallpapers List of wallpapers
 * @param delta_time Time since last update (ms)
 */
void roulette_update(RouletteContext *ctx, WallpaperList *wallpapers, float delta_time);

/**
 * @brief Render current frame of roulette animation
 * @param ctx Roulette context
 * @param wallpapers List of wallpapers
 */
void roulette_render(RouletteContext *ctx, WallpaperList *wallpapers);

/**
 * @brief Cleanup roulette context
 * @param ctx Roulette context to cleanup
 */
void roulette_cleanup(RouletteContext *ctx);

#endif /* ROULETTE_H */
