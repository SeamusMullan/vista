#include "renderer.h"
#include <stdio.h>

Renderer* renderer_init(const Config *config) {
    Renderer *r = malloc(sizeof(Renderer));
    if (!r) return NULL;
    
    r->selected_index = 0;
    r->scroll_offset = 0;
    
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
    // Clear background
    SDL_SetRenderDrawColor(r->renderer, 20, 20, 20, 255);
    SDL_RenderClear(r->renderer);
    
    // Draw thumbnails
    int x = 20 + r->scroll_offset;
    int y = (config->window_height - config->thumbnail_height) / 2;
    int spacing = 20;
    
    for (int i = 0; i < list->count; i++) {
        if (list->items[i].thumb) {
            SDL_Texture *tex = SDL_CreateTextureFromSurface(r->renderer, list->items[i].thumb);
            
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
    
    SDL_RenderPresent(r->renderer);
}

void renderer_select_prev(Renderer *r) {
    if (r->selected_index > 0) {
        r->selected_index--;
    }
}

void renderer_select_next(Renderer *r, int max) {
    if (r->selected_index < max) {
        r->selected_index++;
    }
}

void renderer_cleanup(Renderer *r) {
    if (r->renderer) SDL_DestroyRenderer(r->renderer);
    if (r->window) SDL_DestroyWindow(r->window);
    free(r);
}
