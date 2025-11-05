/**
 * @file main.c
 * @brief Main entry point for vista wallpaper switcher
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "../include/config.h"
#include "../include/thumbnails.h"
#include "../include/renderer.h"
#include "../include/wallpaper.h"
#include "../include/roulette/roulette.h"

#ifdef USE_SHADERS
#include "shader.h"
#endif

/**
 * @brief Print usage information
 */
static void print_usage(const char *prog_name) {
    printf("Usage: %s [options]\n", prog_name);
    printf("Options:\n");
    printf("  -c, --config PATH   Use alternative config file\n");
    printf("  -r, --random        Random wallpaper with roulette animation\n");
    printf("  -h, --help          Show this help message\n");
    printf("  -v, --version       Show version information\n");
}

/**
 * @brief Main function
 */
int main(int argc, char *argv[]) {
    const char *config_path = NULL;
    bool random_mode = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("vista 1.0.0\n");
            return 0;
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--random") == 0) {
            random_mode = true;
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--config") == 0) {
            if (i + 1 < argc) {
                config_path = argv[++i];
            } else {
                fprintf(stderr, "Error: --config requires an argument\n");
                return 1;
            }
        }
    }

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }

    if (!(IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) & (IMG_INIT_PNG | IMG_INIT_JPG))) {
        fprintf(stderr, "SDL_image initialization failed: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    // Load configuration
    Config config;
    if (config_path){
        config = config_path ? config_parse(config_path) : config_default();
    } else {
        // Check if default config file exists
        const char *home = getenv("HOME");
        if (home) {
            char default_config_path[1024];
            snprintf(default_config_path, sizeof(default_config_path), "%s/.config/vista/vista.conf", home);
            
            FILE *f = fopen(default_config_path, "r");
            if (f) {
                fclose(f);
                config = config_parse(default_config_path);
            } else {
                config = config_default();
            }
        } else {
            config = config_default();
        }
    } 
    
    // Scan wallpapers from all configured directories
    printf("Scanning wallpapers in: %s\n", config.wallpaper_dir);
    for (int i = 0; i < config.wallpaper_dirs_count; i++) {
        printf("  Additional directory: %s\n", config.wallpaper_dirs[i]);
    }
    
    WallpaperList wallpapers;
    if (config.wallpaper_dirs_count > 0) {
        wallpapers = wallpaper_list_scan_multiple(&config);
    } else {
        wallpapers = wallpaper_list_scan(config.wallpaper_dir);
    }
    
    if (wallpapers.count == 0) {
        fprintf(stderr, "No wallpapers found\n");
        SDL_Quit();
        return 1;
    }
    
    printf("Found %d wallpapers\n", wallpapers.count);
    
    // Generate thumbnails
    printf("Generating thumbnails...\n");
    wallpaper_list_generate_thumbnails(&wallpapers, &config);

    // Random mode with roulette animation
    if (random_mode) {
        printf("Starting roulette animation...\n");
        RouletteContext *roulette = roulette_init(&config, &wallpapers);
        if (!roulette) {
            fprintf(stderr, "Failed to initialize roulette\n");
            wallpaper_list_free(&wallpapers);
            IMG_Quit();
            SDL_Quit();
            return 1;
        }
        
        int selected_index = roulette_run(roulette, &wallpapers);
        roulette_cleanup(roulette);
        
        // Apply the selected wallpaper
        Wallpaper *selected = wallpaper_list_get(&wallpapers, selected_index);
        if (selected) {
            printf("Applying selected wallpaper: %s\n", selected->path);
            wallpaper_apply(selected->path, &config);
            wallpaper_generate_palette(selected->path, &config);
        }
        
        // Cleanup and exit
        wallpaper_list_free(&wallpapers);
        IMG_Quit();
        SDL_Quit();
        return 0;
    }

    // Initialize renderer
    Renderer *renderer = renderer_init(&config);
    if (!renderer) {
        fprintf(stderr, "Failed to initialize renderer\n");
        wallpaper_list_free(&wallpapers);
        SDL_Quit();
        return 1;
    }

    // Main event loop
    bool running = true;
    SDL_Event event;
    
    while (running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                    
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case SDLK_ESCAPE:
                        case SDLK_q:
                            running = false;
                            break;
                            
                        case SDLK_LEFT:
                        case SDLK_h:
                            renderer_select_prev(renderer, &config);
                            break;
                            
                        case SDLK_RIGHT:
                        case SDLK_l:
                            renderer_select_next(renderer, wallpapers.count - 1, &config);
                            break;
                            
                        case SDLK_UP:
                        case SDLK_k:
                            renderer_select_up(renderer, &config);
                            break;
                            
                        case SDLK_DOWN:
                        case SDLK_j:
                            renderer_select_down(renderer, wallpapers.count - 1, &config);
                            break;
                            
                        case SDLK_g:
                            renderer_toggle_view_mode(renderer);
                            break;
                            
                        case SDLK_f:
                            wallpaper_toggle_favorite(&wallpapers, renderer->selected_index);
                            break;
                            
                        case SDLK_F2:
                            wallpaper_list_toggle_favorites_filter(&wallpapers);
                            renderer->selected_index = 0;
                            printf("Favorites filter: %s\n", wallpapers.show_favorites_only ? "ON" : "OFF");
                            break;
                            
                        case SDLK_SLASH:
                        case SDLK_QUESTION:
                            renderer->show_help = !renderer->show_help;
                            break;
                            
                        case SDLK_RETURN:
                        case SDLK_KP_ENTER:
                            if (renderer->selected_index >= 0) {
                                Wallpaper *wp = wallpaper_list_get(&wallpapers, renderer->selected_index);
                                if (wp) {
                                    printf("Applying wallpaper: %s\n", wp->path);
                                    wallpaper_apply(wp->path, &config);
                                    wallpaper_generate_palette(wp->path, &config);
                                    running = false;
                                }
                            }
                            break;
                    }
                    break;
                    
                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        // Calculate which thumbnail was clicked
                        int visible_count = wallpaper_list_visible_count(&wallpapers);
                        int mouse_x = event.button.x;
                        int mouse_y = event.button.y;
                        
                        if (renderer->view_mode == VIEW_MODE_HORIZONTAL) {
                            int x_pos = 20 + (int)renderer->current_scroll;
                            for (int i = 0; i < visible_count; i++) {
                                if (mouse_x >= x_pos && mouse_x < x_pos + config.thumbnail_width) {
                                    renderer->selected_index = i;
                                    
                                    // Double-click to apply
                                    Wallpaper *wp = wallpaper_list_get(&wallpapers, i);
                                    if (wp) {
                                        printf("Applying wallpaper: %s\n", wp->path);
                                        wallpaper_apply(wp->path, &config);
                                        wallpaper_generate_palette(wp->path, &config);
                                        running = false;
                                    }
                                    break;
                                }
                                x_pos += config.thumbnail_width + 20;
                            }
                        } else {
                            // Grid mode
                            int cols = config.thumbnails_per_row;
                            int start_x = 20;
                            int start_y = 20 + (int)renderer->current_scroll_y;
                            
                            for (int i = 0; i < visible_count; i++) {
                                int col = i % cols;
                                int row = i / cols;
                                int x = start_x + col * (config.thumbnail_width + 20);
                                int y = start_y + row * (config.thumbnail_height + 20);
                                
                                if (mouse_x >= x && mouse_x < x + config.thumbnail_width &&
                                    mouse_y >= y && mouse_y < y + config.thumbnail_height) {
                                    renderer->selected_index = i;
                                    break;
                                }
                            }
                        }
                    }
                    break;
            }
        }
        
        // Render
        renderer_draw_frame(renderer, &wallpapers, &config);
        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    renderer_cleanup(renderer);
    wallpaper_list_free(&wallpapers);
    IMG_Quit();
    SDL_Quit();
    
    fflush(stdout);
    printf("\n");
    return 0;
}
