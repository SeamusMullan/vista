/**
 * @file roulette.c
 * @brief Implementation of CS:GO-style roulette animation
 */

#include "roulette/roulette.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// Easing functions
static float ease_out_cubic(float t) {
    float f = t - 1.0f;
    return f * f * f + 1.0f;
}

static float ease_in_cubic(float t) {
    return t * t * t;
}

static float ease_in_out_quad(float t) {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

RouletteContext* roulette_init(const Config *config, WallpaperList *wallpapers) {
    RouletteContext *ctx = calloc(1, sizeof(RouletteContext));
    if (!ctx) {
        return NULL;
    }
    
    // Create fullscreen window
    SDL_DisplayMode display_mode;
    SDL_GetCurrentDisplayMode(0, &display_mode);
    
    ctx->window = SDL_CreateWindow(
        "Vista - Random Wallpaper",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        display_mode.w,
        display_mode.h,
        SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_SHOWN
    );
    
    if (!ctx->window) {
        fprintf(stderr, "Failed to create window: %s\n", SDL_GetError());
        free(ctx);
        return NULL;
    }
    
    ctx->renderer = SDL_CreateRenderer(ctx->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ctx->renderer) {
        fprintf(stderr, "Failed to create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(ctx->window);
        free(ctx);
        return NULL;
    }
    
    // Initialize animation parameters
    ctx->state = ROULETTE_STATE_STARTING;
    ctx->scroll_position = 0.0f;
    ctx->scroll_velocity = 0.0f;
    ctx->selecting_start_pos = 0.0f;
    
    // Timing (in milliseconds)
    ctx->start_duration = 800.0f;
    ctx->scroll_duration = 2000.0f;
    ctx->slow_duration = 2500.0f;
    ctx->select_duration = 1000.0f;
    ctx->show_duration = 1500.0f;
    
    ctx->max_velocity = 80.0f; // items per second
    
    // Visual parameters
    ctx->item_width = config->thumbnail_width;
    ctx->item_height = config->thumbnail_height;
    ctx->item_spacing = 40;
    ctx->center_x = display_mode.w / 2;
    ctx->center_y = display_mode.h / 2;
    
    // Select random wallpaper and calculate target
    srand(time(NULL));
    int visible_count = wallpaper_list_visible_count(wallpapers);
    ctx->selected_index = rand() % visible_count;
    
    // Calculate number of loops (3-5 full cycles)
    ctx->loops = 3 + (rand() % 3);
    
    // Calculate target position - where the selected item should end up
    // We want the selected item to be centered, so target is selected_index
    // Plus the extra loops
    ctx->target_position = (float)(ctx->loops * visible_count + ctx->selected_index);
    
    ctx->animation_start_time = SDL_GetTicks();
    ctx->state_start_time = ctx->animation_start_time;
    
    printf("Roulette: Selected wallpaper index %d, target position %.2f\n", 
           ctx->selected_index, ctx->target_position);
    
    return ctx;
}

void roulette_update(RouletteContext *ctx, WallpaperList *wallpapers, float delta_time) {
    Uint32 current_time = SDL_GetTicks();
    Uint32 elapsed_total = current_time - ctx->animation_start_time;
    float total_anim_duration = ctx->start_duration + ctx->scroll_duration + ctx->slow_duration;
    
    int visible_count = wallpaper_list_visible_count(wallpapers);
    if (visible_count == 0) return;
    
    switch (ctx->state) {
        case ROULETTE_STATE_STARTING: {
            // Ease into maximum velocity
            Uint32 state_elapsed = current_time - ctx->state_start_time;
            float state_progress = state_elapsed / ctx->start_duration;
            
            if (state_progress >= 1.0f) {
                ctx->state = ROULETTE_STATE_SCROLLING;
                ctx->state_start_time = current_time;
                ctx->scroll_velocity = ctx->max_velocity;
                printf("Roulette: Entering SCROLLING state\n");
            } else {
                ctx->scroll_velocity = ctx->max_velocity * ease_in_cubic(state_progress);
                ctx->scroll_position += ctx->scroll_velocity * (delta_time / 1000.0f);
            }
            break;
        }
            
        case ROULETTE_STATE_SCROLLING: {
            // Maintain maximum velocity
            Uint32 state_elapsed = current_time - ctx->state_start_time;
            float state_progress = state_elapsed / ctx->scroll_duration;
            
            if (state_progress >= 1.0f) {
                ctx->state = ROULETTE_STATE_SLOWING;
                ctx->state_start_time = current_time;
                ctx->selecting_start_pos = ctx->scroll_position;
                printf("Roulette: Entering SLOWING state at position %.2f, target %.2f\n",
                       ctx->scroll_position, ctx->target_position);
            } else {
                ctx->scroll_velocity = ctx->max_velocity;
                ctx->scroll_position += ctx->scroll_velocity * (delta_time / 1000.0f);
            }
            break;
        }
            
        case ROULETTE_STATE_SLOWING: {
            // Decelerate smoothly to land exactly on target
            Uint32 state_elapsed = current_time - ctx->state_start_time;
            float state_progress = state_elapsed / ctx->slow_duration;
            
            if (state_progress >= 1.0f) {
                // Ensure we end exactly at target
                ctx->scroll_position = ctx->target_position;
                ctx->scroll_velocity = 0.0f;
                ctx->state = ROULETTE_STATE_SHOWING;
                ctx->state_start_time = current_time;
                printf("Roulette: Entering SHOWING state - Selected wallpaper %d at position %.2f\n", 
                       ctx->selected_index, ctx->scroll_position);
            } else {
                // Use ease-out curve to decelerate smoothly
                float ease_progress = ease_out_cubic(state_progress);
                
                // Interpolate from start position to target position
                ctx->scroll_position = ctx->selecting_start_pos + 
                    (ctx->target_position - ctx->selecting_start_pos) * ease_progress;
                
                // Calculate instantaneous velocity for visual feedback
                float remaining_distance = ctx->target_position - ctx->scroll_position;
                float remaining_time = ctx->slow_duration - state_elapsed;
                if (remaining_time > 0) {
                    ctx->scroll_velocity = (remaining_distance / (remaining_time / 1000.0f));
                } else {
                    ctx->scroll_velocity = 0.0f;
                }
            }
            break;
        }
            
        case ROULETTE_STATE_SELECTING:
            // This state is no longer used, skip to showing
            ctx->state = ROULETTE_STATE_SHOWING;
            ctx->state_start_time = current_time;
            break;
            
        case ROULETTE_STATE_SHOWING: {
            // Display final selection
            Uint32 state_elapsed = current_time - ctx->state_start_time;
            if (state_elapsed >= (Uint32)ctx->show_duration) {
                ctx->state = ROULETTE_STATE_FINISHED;
                printf("Roulette: Animation complete\n");
            }
            break;
        }
            
        case ROULETTE_STATE_FINISHED:
            // Do nothing, animation is done
            break;
    }
}

void roulette_render(RouletteContext *ctx, WallpaperList *wallpapers) {
    // Clear to dark background
    SDL_SetRenderDrawColor(ctx->renderer, 20, 20, 25, 255);
    SDL_RenderClear(ctx->renderer);
    
    int visible_count = wallpaper_list_visible_count(wallpapers);
    if (visible_count == 0) {
        SDL_RenderPresent(ctx->renderer);
        return;
    }
    
    // Create tiled list that repeats
    int total_item_width = ctx->item_width + ctx->item_spacing;
    
    // Calculate which items to show
    float wrapped_scroll = fmodf(ctx->scroll_position, (float)visible_count);
    if (wrapped_scroll < 0) wrapped_scroll += visible_count;
    
    int base_offset = (int)(wrapped_scroll * total_item_width);
    
    // Draw center indicator (the "selector")
    int indicator_width = ctx->item_width + 20;
    int indicator_height = ctx->item_height + 20;
    SDL_Rect indicator = {
        ctx->center_x - indicator_width / 2,
        ctx->center_y - indicator_height / 2,
        indicator_width,
        indicator_height
    };
    
    // Draw glow effect based on state
    if (ctx->state == ROULETTE_STATE_SHOWING) {
        // Pulsing gold glow when showing result
        Uint32 pulse_time = SDL_GetTicks() - ctx->state_start_time;
        float pulse = 0.5f + 0.5f * sinf((pulse_time / 200.0f) * M_PI);
        SDL_SetRenderDrawColor(ctx->renderer, 
                              (Uint8)(255 * pulse), 
                              (Uint8)(215 * pulse), 
                              (Uint8)(0 * pulse), 
                              255);
    } else {
        // White indicator during animation
        SDL_SetRenderDrawColor(ctx->renderer, 255, 255, 255, 180);
    }
    
    for (int i = 0; i < 3; i++) {
        SDL_Rect glow = indicator;
        glow.x -= (i + 1) * 3;
        glow.y -= (i + 1) * 3;
        glow.w += (i + 1) * 6;
        glow.h += (i + 1) * 6;
        SDL_RenderDrawRect(ctx->renderer, &glow);
    }
    
    // Draw wallpaper thumbnails in a horizontal scrolling strip
    int screen_width = ctx->center_x * 2;
    int items_to_draw = (screen_width / total_item_width) + 4; // Extra for wrapping
    
    for (int i = -2; i < items_to_draw; i++) {
        int item_index = ((int)wrapped_scroll + i) % visible_count;
        if (item_index < 0) item_index += visible_count;
        
        Wallpaper *wp = wallpaper_list_get(wallpapers, item_index);
        if (!wp || !wp->thumb) continue;
        
        // Calculate position
        float fractional_offset = wrapped_scroll - floorf(wrapped_scroll);
        int x = ctx->center_x - (int)(fractional_offset * total_item_width) + (i * total_item_width) - ctx->item_width / 2;
        int y = ctx->center_y - ctx->item_height / 2;
        
        // Calculate distance from center for scaling/fading
        float dist_from_center = fabsf((float)(x + ctx->item_width / 2 - ctx->center_x));
        float max_dist = (float)(screen_width / 2);
        float dist_factor = 1.0f - (dist_from_center / max_dist);
        if (dist_factor < 0.0f) dist_factor = 0.0f;
        
        // Scale based on distance from center
        float scale = 0.6f + 0.4f * dist_factor;
        int scaled_width = (int)(ctx->item_width * scale);
        int scaled_height = (int)(ctx->item_height * scale);
        
        SDL_Rect dest = {
            x + (ctx->item_width - scaled_width) / 2,
            y + (ctx->item_height - scaled_height) / 2,
            scaled_width,
            scaled_height
        };
        
        // Create texture from surface if needed
        SDL_Texture *texture = SDL_CreateTextureFromSurface(ctx->renderer, wp->thumb);
        if (texture) {
            // Set alpha based on distance
            SDL_SetTextureAlphaMod(texture, (Uint8)(255 * dist_factor));
            SDL_RenderCopy(ctx->renderer, texture, NULL, &dest);
            SDL_DestroyTexture(texture);
        }
        
        // Draw border
        SDL_SetRenderDrawColor(ctx->renderer, 100, 100, 100, (Uint8)(255 * dist_factor));
        SDL_RenderDrawRect(ctx->renderer, &dest);
    }
    
    // Draw text information
    if (ctx->state == ROULETTE_STATE_SHOWING || ctx->state == ROULETTE_STATE_FINISHED) {
        // Would draw selected wallpaper name here if we had font rendering
        // For now, just draw a bright overlay
    }
    
    SDL_RenderPresent(ctx->renderer);
}

int roulette_run(RouletteContext *ctx, WallpaperList *wallpapers) {
    bool running = true;
    Uint32 last_time = SDL_GetTicks();
    
    while (running && ctx->state != ROULETTE_STATE_FINISHED) {
        // Handle events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || 
                (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                running = false;
            }
        }
        
        // Update
        Uint32 current_time = SDL_GetTicks();
        float delta_time = (float)(current_time - last_time);
        last_time = current_time;
        
        roulette_update(ctx, wallpapers, delta_time);
        
        // Render
        roulette_render(ctx, wallpapers);
        
        // Cap framerate
        SDL_Delay(16); // ~60 FPS
    }
    
    return ctx->selected_index;
}

void roulette_cleanup(RouletteContext *ctx) {
    if (!ctx) return;
    
    if (ctx->renderer) {
        SDL_DestroyRenderer(ctx->renderer);
    }
    if (ctx->window) {
        SDL_DestroyWindow(ctx->window);
    }
    
    free(ctx);
}
