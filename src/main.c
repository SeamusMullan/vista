/**
 * @file main.c
 * @brief Main entry point for vista wallpaper switcher
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL3/SDL.h>
#ifdef HAVE_SDL_IMAGE
#include <SDL3_image/SDL_image.h>
#endif

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
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }

#ifdef HAVE_SDL_IMAGE
    // SDL3_image initialization is different or not needed for basic formats?
    // Usually IMG_Init is still good practice if using the library
    // But check if it returns 0 on failure or boolean
    // SDL3 conventions usually return true on success
    // However, IMG_Init returns bitmask of loaded support
    // Let's assume it works similarly but check headers if possible.
    // For now, we'll keep it but update the check.
    // Actually, SDL3_image might not need explicit Init for some things, but let's keep it.
    // Note: SDL3_image might return 0 on failure?
    // Let's try standard check.
    
    // Note: SDL3_image headers might be <SDL3_image/SDL_image.h>
#else
    fprintf(stderr, "Note: Built without SDL_image. PNG and BMP files are supported.\n");
    fprintf(stderr, "      For JPEG/JPG support, install SDL3_image.\n");
#endif
    
    // Load configuration
    // config_parse() will automatically check XDG_CONFIG_HOME/vista/vista.conf
    // or ~/.config/vista/vista.conf when path is NULL
    Config config = config_parse(config_path); 
    
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
        SDL_Quit();
        return 0;
    }

    // Initialize renderer
#ifdef USE_SHADERS
    // Use OpenGL renderer if shaders are compiled in AND enabled in config
    Renderer *renderer = NULL;
    GLRenderer *gl_renderer = NULL;
    
    if (config.use_shaders) {
        gl_renderer = gl_renderer_init(&config);
        if (!gl_renderer) {
            fprintf(stderr, "Failed to initialize OpenGL renderer, falling back to SDL\n");
            renderer = renderer_init(&config);
        } else {
            printf("Using OpenGL shader renderer\n");
        }
    } else {
        renderer = renderer_init(&config);
    }
#else
    Renderer *renderer = renderer_init(&config);
#endif
    
#ifdef USE_SHADERS
    if (!renderer && !gl_renderer) {
#else
    if (!renderer) {
#endif
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
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                    
                case SDL_EVENT_KEY_DOWN:
                    switch (event.key.scancode) {
                        case SDL_SCANCODE_ESCAPE:
                        case SDL_SCANCODE_Q:
                            running = false;
                            break;
                            
                        case SDL_SCANCODE_LEFT:
                        case SDL_SCANCODE_H:
#ifdef USE_SHADERS
                            if (gl_renderer) {
                                if (gl_renderer->selected_index > 0) {
                                    gl_renderer->selected_index--;
                                    gl_renderer->target_scroll += 220;
                                }
                            } else
#endif
                            renderer_select_prev(renderer, &config);
                            break;
                            
                        case SDL_SCANCODE_RIGHT:
                        case SDL_SCANCODE_L:
#ifdef USE_SHADERS
                            if (gl_renderer) {
                                if (gl_renderer->selected_index < wallpapers.count - 1) {
                                    gl_renderer->selected_index++;
                                    gl_renderer->target_scroll -= 220;
                                }
                            } else
#endif
                            renderer_select_next(renderer, wallpapers.count - 1, &config);
                            break;
                            
                        case SDL_SCANCODE_UP:
                        case SDL_SCANCODE_K:
                            renderer_select_up(renderer, &config);
                            break;
                            
                        case SDL_SCANCODE_DOWN:
                        case SDL_SCANCODE_J:
                            renderer_select_down(renderer, wallpapers.count - 1, &config);
                            break;
                            
                        case SDL_SCANCODE_G:
                            renderer_toggle_view_mode(renderer);
                            break;
                            
                        case SDL_SCANCODE_F:
#ifdef USE_SHADERS
                            if (gl_renderer) {
                                wallpaper_toggle_favorite(&wallpapers, gl_renderer->selected_index);
                            } else
#endif
                            wallpaper_toggle_favorite(&wallpapers, renderer->selected_index);
                            break;
                            
                        case SDL_SCANCODE_F2:
                            wallpaper_list_toggle_favorites_filter(&wallpapers);
#ifdef USE_SHADERS
                            if (gl_renderer) {
                                gl_renderer->selected_index = 0;
                            } else
#endif
                            renderer->selected_index = 0;
                            printf("Favorites filter: %s\n", wallpapers.show_favorites_only ? "ON" : "OFF");
                            break;
                            
                        case SDL_SCANCODE_SLASH:
                            renderer->show_help = !renderer->show_help;
                            break;
                            
                        case SDL_SCANCODE_RETURN:
                        case SDL_SCANCODE_KP_ENTER: {
                            int sel_idx = 0;
#ifdef USE_SHADERS
                            sel_idx = gl_renderer ? gl_renderer->selected_index : renderer->selected_index;
#else
                            sel_idx = renderer->selected_index;
#endif
                            if (sel_idx >= 0) {
                                Wallpaper *wp = wallpaper_list_get(&wallpapers, sel_idx);
                                if (wp) {
                                    printf("Applying wallpaper: %s\n", wp->path);
                                    wallpaper_apply(wp->path, &config);
                                    wallpaper_generate_palette(wp->path, &config);
                                    running = false;
                                }
                            }
                            break;
                        }
                    }
                    break;
                    
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        // Calculate which thumbnail was clicked
                        int visible_count = wallpaper_list_visible_count(&wallpapers);
                        float mouse_x = event.button.x;
                        float mouse_y = event.button.y;
                        
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
#ifdef USE_SHADERS
        if (gl_renderer) {
            gl_renderer_draw_frame(gl_renderer, &wallpapers, &config);
        } else
#endif
        renderer_draw_frame(renderer, &wallpapers, &config);
        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
#ifdef USE_SHADERS
    if (gl_renderer) {
        gl_renderer_cleanup(gl_renderer);
    } else
#endif
    renderer_cleanup(renderer);
    wallpaper_list_free(&wallpapers);
    SDL_Quit();
    
    fflush(stdout);
    printf("\n");
    return 0;
}
