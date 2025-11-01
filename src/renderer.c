#include "renderer.h"
#include <stdio.h>

Renderer* renderer_init(const Config *config) {
    Renderer *r = malloc(sizeof(Renderer));
    if (!r) return NULL;
    
    r->selected_index = 0;
    r->scroll_offset = 0;
    r->target_scroll = 0.0f;
    r->current_scroll = 0.0f;
    r->view_mode = VIEW_MODE_HORIZONTAL;
    r->grid_scroll_y = 0;
    r->target_scroll_y = 0.0f;
    r->current_scroll_y = 0.0f;
    r->search_mode = false;
    r->show_help = false;
    
    // Create window
    r->window = SDL_CreateWindow(
        "vista - wallpaper switcher",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        config->window_width,
        config->window_height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS
    );
    
    if (!r->window) {
        fprintf(stderr, "Failed to create window: %s\n", SDL_GetError());
        free(r);
        return NULL;
    }
    
    r->renderer = SDL_CreateRenderer(r->window, -1, SDL_RENDERER_ACCELERATED);
    if (!r->renderer) {
        fprintf(stderr, "Failed to create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(r->window);
        free(r);
        return NULL;
    }
    
    return r;
}

void renderer_draw_frame(Renderer *r, const WallpaperList *list, const Config *config) {
    // Smooth scroll animation (lerp)
    const float smoothness = 0.15f;
    r->current_scroll += (r->target_scroll - r->current_scroll) * smoothness;
    r->current_scroll_y += (r->target_scroll_y - r->current_scroll_y) * smoothness;
    
    // Clear background
    SDL_SetRenderDrawColor(r->renderer, 20, 20, 20, 255);
    SDL_RenderClear(r->renderer);
    
    int visible_count = wallpaper_list_visible_count(list);
    
    if (r->view_mode == VIEW_MODE_HORIZONTAL) {
        // Draw horizontal strip
        int x = 20 + (int)r->current_scroll;
        int y = (config->window_height - config->thumbnail_height) / 2;
        int spacing = 20;
        
        for (int i = 0; i < visible_count; i++) {
            Wallpaper *wp = wallpaper_list_get((WallpaperList*)list, i);
            if (wp && wp->thumb) {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(r->renderer, wp->thumb);
                
                SDL_Rect dest = {x, y, config->thumbnail_width, config->thumbnail_height};
                SDL_RenderCopy(r->renderer, tex, NULL, &dest);
                
                // Highlight selected
                if (i == r->selected_index) {
                    SDL_SetRenderDrawColor(r->renderer, 100, 200, 255, 255);
                    SDL_RenderDrawRect(r->renderer, &dest);
                    SDL_RenderDrawRect(r->renderer, &(SDL_Rect){dest.x-1, dest.y-1, dest.w+2, dest.h+2});
                    SDL_RenderDrawRect(r->renderer, &(SDL_Rect){dest.x-2, dest.y-2, dest.w+4, dest.h+4});
                }
                
                SDL_DestroyTexture(tex);
            }
            
            x += config->thumbnail_width + spacing;
        }
    } else {
        // Draw grid view
        int spacing = 20;
        int cols = config->thumbnails_per_row;
        int start_x = 20;
        int start_y = 20 + (int)r->current_scroll_y;
        
        for (int i = 0; i < visible_count; i++) {
            Wallpaper *wp = wallpaper_list_get((WallpaperList*)list, i);
            if (wp && wp->thumb) {
                int col = i % cols;
                int row = i / cols;
                
                int x = start_x + col * (config->thumbnail_width + spacing);
                int y = start_y + row * (config->thumbnail_height + spacing);
                
                SDL_Texture *tex = SDL_CreateTextureFromSurface(r->renderer, wp->thumb);
                
                SDL_Rect dest = {x, y, config->thumbnail_width, config->thumbnail_height};
                SDL_RenderCopy(r->renderer, tex, NULL, &dest);
                
                // Highlight selected
                if (i == r->selected_index) {
                    SDL_SetRenderDrawColor(r->renderer, 100, 200, 255, 255);
                    SDL_RenderDrawRect(r->renderer, &dest);
                    SDL_RenderDrawRect(r->renderer, &(SDL_Rect){dest.x-1, dest.y-1, dest.w+2, dest.h+2});
                    SDL_RenderDrawRect(r->renderer, &(SDL_Rect){dest.x-2, dest.y-2, dest.w+4, dest.h+4});
                }
                
                SDL_DestroyTexture(tex);
            }
        }
    }
    
    // Draw help overlay if enabled
    if (r->show_help) {
        renderer_draw_help_overlay(r);
    }
    
    SDL_RenderPresent(r->renderer);
}

void renderer_select_prev(Renderer *r, const Config *config) {
    if (r->view_mode == VIEW_MODE_HORIZONTAL) {
        if (r->selected_index > 0) {
            r->selected_index--;
            r->target_scroll += 220; // thumbnail width + spacing
        }
    } else {
        // In grid mode, move left
        if (r->selected_index > 0) {
            r->selected_index--;
        }
    }
}

void renderer_select_next(Renderer *r, int max, const Config *config) {
    if (r->view_mode == VIEW_MODE_HORIZONTAL) {
        if (r->selected_index < max) {
            r->selected_index++;
            r->target_scroll -= 220; // thumbnail width + spacing
        }
    } else {
        // In grid mode, move right
        if (r->selected_index < max) {
            r->selected_index++;
        }
    }
}

void renderer_select_up(Renderer *r, const Config *config) {
    if (r->view_mode == VIEW_MODE_GRID) {
        int cols = config->thumbnails_per_row;
        if (r->selected_index >= cols) {
            r->selected_index -= cols;
            r->target_scroll_y += config->thumbnail_height + 20;
        }
    }
}

void renderer_select_down(Renderer *r, int max, const Config *config) {
    if (r->view_mode == VIEW_MODE_GRID) {
        int cols = config->thumbnails_per_row;
        if (r->selected_index + cols <= max) {
            r->selected_index += cols;
            r->target_scroll_y -= config->thumbnail_height + 20;
        }
    }
}

void renderer_toggle_view_mode(Renderer *r) {
    r->view_mode = (r->view_mode == VIEW_MODE_HORIZONTAL) ? VIEW_MODE_GRID : VIEW_MODE_HORIZONTAL;
    r->target_scroll = 0;
    r->current_scroll = 0;
    r->target_scroll_y = 0;
    r->current_scroll_y = 0;
}

void renderer_draw_help_overlay(Renderer *r) {
    // Draw semi-transparent background
    SDL_SetRenderDrawBlendMode(r->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r->renderer, 0, 0, 0, 200);
    SDL_Rect overlay = {100, 50, 600, 500};
    SDL_RenderFillRect(r->renderer, &overlay);
    
    // Draw border
    SDL_SetRenderDrawColor(r->renderer, 100, 200, 255, 255);
    SDL_RenderDrawRect(r->renderer, &overlay);
    
    // Note: Text rendering would require SDL_ttf
    // For now, this creates the overlay box
    // In a full implementation, you would render text here showing:
    // Arrow Keys / hjkl - Navigate
    // Enter - Apply wallpaper
    // g - Toggle grid/horizontal view
    // f - Toggle favorite
    // F2 - Filter favorites
    // / or ? - Toggle help
    // q or Esc - Quit
}

void renderer_cleanup(Renderer *r) {
    if (r->renderer) SDL_DestroyRenderer(r->renderer);
    if (r->window) SDL_DestroyWindow(r->window);
    free(r);
}
