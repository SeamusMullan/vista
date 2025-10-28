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
    printf("  -h, --help          Show this help message\n");
    printf("  -v, --version       Show version information\n");
}

/**
 * @brief Main function
 */
int main(int argc, char *argv[]) {
    const char *config_path = NULL;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("vista 1.0.0\n");
            return 0;
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
    Config config = config_path ? config_parse(config_path) : config_default();
    
    // Scan wallpapers
    printf("Scanning wallpapers in: %s\n", config.wallpaper_dir);
    WallpaperList wallpapers = wallpaper_list_scan(config.wallpaper_dir);
    
    if (wallpapers.count == 0) {
        fprintf(stderr, "No wallpapers found in %s\n", config.wallpaper_dir);
        SDL_Quit();
        return 1;
    }
    
    printf("Found %d wallpapers\n", wallpapers.count);
    
    // Generate thumbnails
    printf("Generating thumbnails...\n");
    wallpaper_list_generate_thumbnails(&wallpapers, &config);

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
                            running = false;
                            break;
                            
                        case SDLK_LEFT:
                            renderer_select_prev(renderer);
                            break;
                            
                        case SDLK_RIGHT:
                            renderer_select_next(renderer, wallpapers.count - 1);
                            break;
                            
                        case SDLK_RETURN:
                        case SDLK_KP_ENTER:
                            if (renderer->selected_index >= 0 && 
                                renderer->selected_index < wallpapers.count) {
                                const char *selected = wallpapers.items[renderer->selected_index].path;
                                printf("Applying wallpaper: %s\n", selected);
                                wallpaper_apply(selected, &config);
                                wallpaper_generate_palette(selected, &config);
                                running = false;
                            }
                            break;
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
    
    return 0;
}
