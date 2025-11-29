/**
 * @file roulette.c
 * @brief Implementation of CS:GO-style roulette animation
 */

#include "roulette/roulette.h"
#ifdef HAVE_SDL_MIXER
#include "roulette/sound.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <SDL3/SDL.h>

// Easing functions
static float ease_out_cubic(float t) {
    float f = t - 1.0f;
    return f * f * f + 1.0f;
}

// Gentler ease-out for smoother deceleration
static float ease_out_quad(float t) {
    return t * (2.0f - t);
}

// Very smooth exponential ease-out - feels most natural
static float ease_out_expo(float t) {
    if (t >= 1.0f) return 1.0f;
    return 1.0f - powf(2.0f, -10.0f * t);
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
    
#ifdef HAVE_SDL_MIXER
    // Initialize SDL3_mixer
    // Create a mixer
    SDL_AudioSpec spec;
    spec.freq = 44100;
    spec.format = SDL_AUDIO_S16;
    spec.channels = 2;

    ctx->mixer = MIX_CreateMixer(&spec);
    if (!ctx->mixer) {
        fprintf(stderr, "SDL3_mixer could not initialize: %s\n", SDL_GetError());
        // Continue anyway - audio is optional
    } else {
        // Load or generate sound effects
        const char *audio_dir = (config->audio_dir[0] != '\0') ? config->audio_dir : NULL;

        if (audio_dir) {
            printf("Looking for audio files in: %s\n", audio_dir);
        }

        ctx->tick_sound = roulette_sound_load_tick(ctx->mixer, audio_dir);
        ctx->select_sound = roulette_sound_load_select(ctx->mixer, audio_dir);

        if (!ctx->tick_sound || !ctx->select_sound) {
            fprintf(stderr, "Warning: Could not load/generate sound effects\n");
        } else {
            printf("Audio enabled: Sound effects ready\n");
        }

        // Create tracks for playing sounds
        ctx->tick_track = MIX_CreateTrack(ctx->mixer);
        ctx->select_track = MIX_CreateTrack(ctx->mixer);
    }
#else
    printf("Built without SDL_mixer - no audio\n");
#endif
    
    ctx->last_item_index = -1;
    
    // Create fullscreen window
    SDL_DisplayID display_id = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode *display_mode = SDL_GetDesktopDisplayMode(display_id);
    int w = 1920, h = 1080;
    if (display_mode) {
        w = display_mode->w;
        h = display_mode->h;
    }

    ctx->window = SDL_CreateWindow(
        "Vista - Random Wallpaper",
        w,
        h,
        SDL_WINDOW_FULLSCREEN
    );
    
    if (!ctx->window) {
        fprintf(stderr, "Failed to create window: %s\n", SDL_GetError());
        free(ctx);
        return NULL;
    }
    
    ctx->renderer = SDL_CreateRenderer(ctx->window, NULL);
    if (!ctx->renderer) {
        fprintf(stderr, "Failed to create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(ctx->window);
        free(ctx);
        return NULL;
    }
    
    SDL_SetRenderVSync(ctx->renderer, 1);

    // Initialize animation parameters
    ctx->state = ROULETTE_STATE_STARTING;
    ctx->scroll_position = 0.0f;
    ctx->scroll_velocity = 0.0f;
    ctx->selecting_start_pos = 0.0f;
    
    // Timing (in milliseconds) - use config values
    ctx->start_duration = (float)config->roulette_start_duration;
    ctx->scroll_duration = (float)config->roulette_scroll_duration;
    ctx->slow_duration = (float)config->roulette_slow_duration;
    ctx->select_duration = 1000.0f; // Not used anymore but keep for compatibility
    ctx->show_duration = (float)config->roulette_show_duration;
    
    ctx->max_velocity = config->roulette_max_velocity; // items per second
    
    // Visual parameters
    ctx->item_width = config->thumbnail_width;
    ctx->item_height = config->thumbnail_height;
    ctx->item_spacing = 40;
    ctx->center_x = w / 2;
    ctx->center_y = h / 2;
    
    // Select random wallpaper and calculate target
    srand(time(NULL));
    int visible_count = wallpaper_list_visible_count(wallpapers);
    ctx->selected_index = rand() % visible_count;
    
    // Calculate minimum distance needed based on animation duration and velocity
    // Distance during acceleration (triangular area: 0.5 * max_vel * time)
    float accel_distance = 0.5f * ctx->max_velocity * (ctx->start_duration / 1000.0f);
    // Distance during constant velocity (reduced to start slowing earlier)
    float scroll_distance = ctx->max_velocity * (ctx->scroll_duration / 1000.0f) * 0.7f;
    // Distance during deceleration - give more room for gradual slowdown
    float decel_distance = ctx->max_velocity * (ctx->slow_duration / 1000.0f) * 0.6f;
    
    float total_distance = accel_distance + scroll_distance + decel_distance;
    
    // Calculate loops needed to cover at least this distance
    int min_loops = (int)(total_distance / visible_count);
    if (min_loops < 2) min_loops = 2; // At least 2 loops
    
    // Add some randomness (0-1 extra loop for more control)
    ctx->loops = min_loops + (rand() % 2);
    
    // Calculate target position - where the selected item should end up
    // We want the selected item to be centered, so target is selected_index
    // Plus the extra loops
    ctx->target_position = (float)(ctx->loops * visible_count + ctx->selected_index);
    
    ctx->animation_start_time = SDL_GetTicks();
    ctx->state_start_time = ctx->animation_start_time;
    
    printf("Roulette: Selected wallpaper index %d, target position %.2f (loops: %d, distance: %.1f)\n", 
           ctx->selected_index, ctx->target_position, ctx->loops, total_distance);
    
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
            float state_progress = fminf(state_elapsed / ctx->start_duration, 1.0f);
            
            if (state_progress >= 1.0f) {
                ctx->state = ROULETTE_STATE_SCROLLING;
                ctx->state_start_time = current_time;
                ctx->scroll_velocity = ctx->max_velocity;
                printf("Roulette: Entering SCROLLING state at position %.2f\n", ctx->scroll_position);
            } else {
                // Smooth acceleration using ease-in curve
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
            float state_progress = fminf(state_elapsed / ctx->slow_duration, 1.0f);
            
            if (state_progress >= 1.0f) {
                // Don't snap - let it smoothly continue into SHOWING state
                // The lerp in SHOWING will handle final alignment
                ctx->scroll_velocity = 0.0f;
                ctx->state = ROULETTE_STATE_SHOWING;
                ctx->state_start_time = current_time;

#ifdef HAVE_SDL_MIXER
                // Stop any playing tick sounds and play selection sound
                if (ctx->select_sound && ctx->tick_track && ctx->select_track) {
                    MIX_StopTrack(ctx->tick_track, 0);
                    MIX_SetTrackAudio(ctx->select_track, ctx->select_sound);
                    MIX_PlayTrack(ctx->select_track, 0);
                }
#endif
                
                printf("Roulette: Entering SHOWING state - Selected wallpaper %d at position %.6f (target: %.6f)\n", 
                       ctx->selected_index, ctx->scroll_position, ctx->target_position);
            } else {
                // Use exponential ease-out for very smooth, natural deceleration
                // This feels most like a real physical object coming to rest
                float ease_progress = ease_out_expo(state_progress);
                
                // Store previous position to calculate velocity
                float prev_position = ctx->scroll_position;
                
                // Interpolate from start position to target position using high precision
                ctx->scroll_position = ctx->selecting_start_pos + 
                    (ctx->target_position - ctx->selecting_start_pos) * ease_progress;
                
                // Calculate actual velocity based on position change
                // This gives consistent velocity for visual feedback without affecting movement
                float position_delta = ctx->scroll_position - prev_position;
                ctx->scroll_velocity = fabsf(position_delta / (delta_time / 1000.0f));
            }
            break;
        }
            
        case ROULETTE_STATE_SELECTING:
            // This state is no longer used, skip to showing
            ctx->state = ROULETTE_STATE_SHOWING;
            ctx->state_start_time = current_time;
            break;
            
        case ROULETTE_STATE_SHOWING: {
            // Display final selection with smooth lerp to perfect alignment
            // Apply a gentle lerp to ensure perfect centering
            float lerp_speed = 10.0f; // Higher = faster convergence
            float distance_to_target = ctx->target_position - ctx->scroll_position;
            
            // Only lerp if we're not already exactly at target
            if (fabsf(distance_to_target) > 0.0001f) {
                float lerp_factor = 1.0f - expf(-lerp_speed * (delta_time / 1000.0f));
                ctx->scroll_position += distance_to_target * lerp_factor;
            } else {
                // Snap to exact target once we're close enough
                ctx->scroll_position = ctx->target_position;
            }
            
            Uint32 state_elapsed = current_time - ctx->state_start_time;
            if (state_elapsed >= (Uint32)ctx->show_duration) {
                // Final snap to exact position
                ctx->scroll_position = ctx->target_position;
                ctx->state = ROULETTE_STATE_FINISHED;
                printf("Roulette: Animation complete at exact position %.6f\n", ctx->scroll_position);
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
    
    // Check if we've moved to a new item (for tick sound)
    int current_item = (int)floorf(ctx->scroll_position) % visible_count;
    if (current_item < 0) current_item += visible_count;
    
    if (current_item != ctx->last_item_index) {
#ifdef HAVE_SDL_MIXER
        if (ctx->tick_sound && ctx->tick_track) {
            // Only play tick during scrolling/slowing, not during showing
            if (ctx->state == ROULETTE_STATE_SCROLLING ||
                (ctx->state == ROULETTE_STATE_SLOWING && ctx->scroll_velocity > 5.0f)) {
                // Stop current tick and play new one for snappier feel
                MIX_StopTrack(ctx->tick_track, 0);
                MIX_SetTrackAudio(ctx->tick_track, ctx->tick_sound);
                MIX_PlayTrack(ctx->tick_track, 0);
            }
        }
#endif
        ctx->last_item_index = current_item;
    }
    
    // Create tiled list that repeats
    int total_item_width = ctx->item_width + ctx->item_spacing;
    
    // Calculate which items to show
    float wrapped_scroll = fmodf(ctx->scroll_position, (float)visible_count);
    if (wrapped_scroll < 0) wrapped_scroll += visible_count;
    
    int base_offset = (int)(wrapped_scroll * total_item_width);
    
    // Draw center indicator (the "selector") - perfectly centered
    int indicator_width = ctx->item_width + 20;
    int indicator_height = ctx->item_height + 20;
    SDL_FRect indicator = {
        (float)(ctx->center_x - indicator_width / 2),
        (float)(ctx->center_y - indicator_height / 2),
        (float)indicator_width,
        (float)indicator_height
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
    
    // Draw multiple glow layers for depth
    for (int i = 0; i < 3; i++) {
        SDL_FRect glow = indicator;
        glow.x -= (i + 1) * 3;
        glow.y -= (i + 1) * 3;
        glow.w += (i + 1) * 6;
        glow.h += (i + 1) * 6;
        SDL_RenderRect(ctx->renderer, &glow);
    }
    
    // Draw wallpaper thumbnails in a horizontal scrolling strip
    int screen_width = ctx->center_x * 2;
    int items_per_side = (screen_width / total_item_width) / 2 + 3; // Items on each side plus extra
    
    // Use the actual scroll position for perfect alignment
    // wrapped_scroll gives us which item + fractional offset
    float fractional_offset = wrapped_scroll - floorf(wrapped_scroll);
    int base_index = (int)floorf(wrapped_scroll);
    
    for (int i = -items_per_side; i <= items_per_side; i++) {
        int item_index = (base_index + i) % visible_count;
        if (item_index < 0) item_index += visible_count;
        
        Wallpaper *wp = wallpaper_list_get(wallpapers, item_index);
        if (!wp || !wp->thumb) continue;
        
        // Calculate position - use floating point for perfect alignment
        // The center should show the item at scroll_position
        // Items are offset by (i - fractional_offset) from center
        float x_offset = (i - fractional_offset) * total_item_width;
        float x_float = ctx->center_x + x_offset - ctx->item_width / 2.0f;
        float y_float = ctx->center_y - ctx->item_height / 2.0f;
        
        // Calculate distance from center for scaling/fading (use float position)
        float item_center_x = x_float + ctx->item_width / 2.0f;
        float dist_from_center = fabsf(item_center_x - ctx->center_x);
        float max_dist = (float)(screen_width / 2);
        float dist_factor = 1.0f - (dist_from_center / max_dist);
        if (dist_factor < 0.0f) dist_factor = 0.0f;
        
        // Scale based on distance from center
        float scale = 0.6f + 0.4f * dist_factor;
        float scaled_width = ctx->item_width * scale;
        float scaled_height = ctx->item_height * scale;
        
        // Round to integers only at the final step for SDL_Rect
        SDL_FRect dest = {
            x_float + (ctx->item_width - scaled_width) / 2.0f,
            y_float + (ctx->item_height - scaled_height) / 2.0f,
            scaled_width,
            scaled_height
        };
        
        // Create texture from surface if needed
        SDL_Texture *texture = SDL_CreateTextureFromSurface(ctx->renderer, wp->thumb);
        if (texture) {
            // Set alpha based on distance
            SDL_SetTextureAlphaMod(texture, (Uint8)(255 * dist_factor));
            SDL_RenderTexture(ctx->renderer, texture, NULL, &dest);
            SDL_DestroyTexture(texture);
        }
        
        // Draw border
        SDL_SetRenderDrawColor(ctx->renderer, 100, 100, 100, (Uint8)(255 * dist_factor));
        SDL_RenderRect(ctx->renderer, &dest);
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
            if (event.type == SDL_EVENT_QUIT ||
                (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE)) {
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

#ifdef HAVE_SDL_MIXER
    if (ctx->tick_track) {
        MIX_DestroyTrack(ctx->tick_track);
    }
    if (ctx->select_track) {
        MIX_DestroyTrack(ctx->select_track);
    }
    if (ctx->tick_sound) {
        roulette_sound_free(ctx->tick_sound);
    }
    if (ctx->select_sound) {
        roulette_sound_free(ctx->select_sound);
    }
    if (ctx->mixer) {
        MIX_DestroyMixer(ctx->mixer);
    }
#endif

    if (ctx->renderer) {
        SDL_DestroyRenderer(ctx->renderer);
    }
    if (ctx->window) {
        SDL_DestroyWindow(ctx->window);
    }

    free(ctx);
}
