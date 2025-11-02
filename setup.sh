#!/bin/bash
# vista project setup script
# Run this in an empty directory with existing LICENSE and README.md

set -e  # Exit on error

echo "Setting up vista wallpaper switcher project..."

# Color output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Create directory structure
echo -e "${BLUE}Creating directory structure...${NC}"
mkdir -p src include shaders docs build .cache config

# Create CMakeLists.txt
echo -e "${BLUE}Creating CMakeLists.txt...${NC}"
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.10)
project(vista VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -march=native")

# Options
option(USE_SHADERS "Enable OpenGL shader support" OFF)
option(BUILD_DOCS "Build documentation with Doxygen" OFF)

# Find SDL2
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
)

# Source files
set(SOURCES
    src/main.c
    src/config.c
    src/thumbnails.c
    src/renderer.c
    src/wallpaper.c
)

# Add shader support if enabled
if(USE_SHADERS)
    find_package(OpenGL REQUIRED)
    find_package(GLEW REQUIRED)
    list(APPEND SOURCES src/shader.c)
    add_definitions(-DUSE_SHADERS)
endif()

# Create executable
add_executable(vista ${SOURCES})

# Link libraries
target_link_libraries(vista
    SDL2::SDL2
    SDL2_image::SDL2_image
    m
)

if(USE_SHADERS)
    target_link_libraries(vista
        OpenGL::GL
        GLEW::GLEW
    )
endif()

# Install targets
install(TARGETS vista DESTINATION bin)
install(FILES config/vista.conf.example DESTINATION share/vista)
install(DIRECTORY shaders DESTINATION share/vista)

# Documentation
if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        add_custom_target(docs
            COMMAND ${CMAKE_SOURCE_DIR}/docs/generate_docs.sh
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Generating documentation with Doxygen"
        )
    endif()
endif()

# Uninstall target
add_custom_target(uninstall
    COMMAND xargs rm -f < install_manifest.txt
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
EOF

# Create header files
echo -e "${BLUE}Creating header files...${NC}"

cat > include/config.h << 'EOF'
/**
 * @file config.h
 * @brief Configuration parsing and management
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

#define MAX_PATH 256

/**
 * @brief Configuration structure
 */
typedef struct {
    char wallpaper_dir[MAX_PATH];    /**< Directory containing wallpapers */
    char feh_command[MAX_PATH];      /**< Command to set wallpaper */
    char palette_script[MAX_PATH];   /**< Script to generate color palette */
    int thumbnail_width;              /**< Thumbnail width in pixels */
    int thumbnail_height;             /**< Thumbnail height in pixels */
    int window_width;                 /**< Window width */
    int window_height;                /**< Window height */
    bool use_shaders;                 /**< Enable shader rendering */
    int thumbnails_per_row;           /**< Number of thumbnails per row */
} Config;

/**
 * @brief Parse configuration file
 * @param path Path to config file
 * @return Parsed configuration
 */
Config config_parse(const char *path);

/**
 * @brief Get default configuration
 * @return Default configuration values
 */
Config config_default(void);

/**
 * @brief Print configuration (for debugging)
 * @param config Configuration to print
 */
void config_print(const Config *config);

#endif /* CONFIG_H */
EOF

cat > include/thumbnails.h << 'EOF'
/**
 * @file thumbnails.h
 * @brief Thumbnail generation and caching
 */

#ifndef THUMBNAILS_H
#define THUMBNAILS_H

#include <SDL2/SDL.h>
#include "config.h"

/**
 * @brief Wallpaper structure
 */
typedef struct {
    char *path;           /**< Full path to wallpaper file */
    char *name;           /**< Filename without path */
    SDL_Surface *thumb;   /**< Loaded thumbnail surface */
} Wallpaper;

/**
 * @brief List of wallpapers
 */
typedef struct {
    Wallpaper *items;     /**< Array of wallpapers */
    int count;            /**< Number of wallpapers */
    int capacity;         /**< Allocated capacity */
} WallpaperList;

/**
 * @brief Scan directory for wallpapers
 * @param dir Directory path
 * @return List of found wallpapers
 */
WallpaperList wallpaper_list_scan(const char *dir);

/**
 * @brief Generate thumbnails for all wallpapers
 * @param list Wallpaper list
 * @param config Configuration
 */
void wallpaper_list_generate_thumbnails(WallpaperList *list, const Config *config);

/**
 * @brief Load or create cached thumbnail
 * @param path Original image path
 * @param width Thumbnail width
 * @param height Thumbnail height
 * @return Loaded thumbnail surface
 */
SDL_Surface* thumbnail_load_or_cache(const char *path, int width, int height);

/**
 * @brief Free wallpaper list
 * @param list List to free
 */
void wallpaper_list_free(WallpaperList *list);

#endif /* THUMBNAILS_H */
EOF

cat > include/renderer.h << 'EOF'
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
EOF

cat > include/wallpaper.h << 'EOF'
/**
 * @file wallpaper.h
 * @brief Wallpaper application logic
 */

#ifndef WALLPAPER_H
#define WALLPAPER_H

#include "config.h"

/**
 * @brief Apply wallpaper using feh
 * @param path Path to wallpaper file
 * @param config Configuration
 * @return 0 on success, -1 on error
 */
int wallpaper_apply(const char *path, const Config *config);

/**
 * @brief Run palette generation script
 * @param wallpaper_path Path to wallpaper
 * @param config Configuration
 * @return 0 on success, -1 on error
 */
int wallpaper_generate_palette(const char *wallpaper_path, const Config *config);

#endif /* WALLPAPER_H */
EOF

cat > include/shader.h << 'EOF'
/**
 * @file shader.h
 * @brief OpenGL shader support (optional)
 */

#ifndef SHADER_H
#define SHADER_H

#ifdef USE_SHADERS

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include "config.h"
#include "thumbnails.h"

/**
 * @brief OpenGL renderer state
 */
typedef struct {
    SDL_Window *window;
    SDL_GLContext gl_context;
    GLuint shader_program;
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
} GLRenderer;

/**
 * @brief Initialize OpenGL renderer
 * @param config Configuration
 * @return Initialized GL renderer
 */
GLRenderer* gl_renderer_init(const Config *config);

/**
 * @brief Render frame with OpenGL
 * @param r GL renderer
 * @param list Wallpaper list
 * @param selected_index Currently selected index
 */
void gl_renderer_draw_frame(GLRenderer *r, const WallpaperList *list, int selected_index);

/**
 * @brief Load and compile shader
 * @param path Shader file path
 * @param type Shader type (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER)
 * @return Compiled shader ID
 */
GLuint shader_load(const char *path, GLenum type);

/**
 * @brief Clean up GL renderer
 * @param r Renderer to free
 */
void gl_renderer_cleanup(GLRenderer *r);

#endif /* USE_SHADERS */

#endif /* SHADER_H */
EOF

# Create source files with skeleton implementations
echo -e "${BLUE}Creating source files...${NC}"

cat > src/main.c << 'EOF'
/**
 * @file main.c
 * @brief Main entry point for vista wallpaper switcher
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "config.h"
#include "thumbnails.h"
#include "renderer.h"
#include "wallpaper.h"

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
EOF

cat > src/config.c << 'EOF'
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>

Config config_default(void) {
    Config config = {0};
    
    // Get home directory
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        home = pw->pw_dir;
    }
    
    snprintf(config.wallpaper_dir, MAX_PATH, "%s/Pictures/Wallpapers", home);
    snprintf(config.feh_command, MAX_PATH, "feh --bg-scale");
    snprintf(config.palette_script, MAX_PATH, "");
    
    config.thumbnail_width = 200;
    config.thumbnail_height = 150;
    config.window_width = 1200;
    config.window_height = 300;
    config.use_shaders = false;
    config.thumbnails_per_row = 5;
    
    return config;
}

Config config_parse(const char *path) {
    Config config = config_default();
    
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Warning: Could not open config file %s, using defaults\n", path);
        return config;
    }
    
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n') continue;
        
        // Parse key=value
        char key[128], value[384];
        if (sscanf(line, " %127[^=] = %383[^\n]", key, value) == 2) {
            // Trim whitespace
            char *k = key, *v = value;
            while (*k == ' ') k++;
            while (*v == ' ') v++;
            
            if (strcmp(k, "wallpaper_dir") == 0) {
                strncpy(config.wallpaper_dir, v, MAX_PATH - 1);
            } else if (strcmp(k, "feh_command") == 0) {
                strncpy(config.feh_command, v, MAX_PATH - 1);
            } else if (strcmp(k, "palette_script") == 0) {
                strncpy(config.palette_script, v, MAX_PATH - 1);
            } else if (strcmp(k, "thumbnail_width") == 0) {
                config.thumbnail_width = atoi(v);
            } else if (strcmp(k, "thumbnail_height") == 0) {
                config.thumbnail_height = atoi(v);
            } else if (strcmp(k, "window_width") == 0) {
                config.window_width = atoi(v);
            } else if (strcmp(k, "window_height") == 0) {
                config.window_height = atoi(v);
            } else if (strcmp(k, "use_shaders") == 0) {
                config.use_shaders = (strcmp(v, "true") == 0 || strcmp(v, "1") == 0);
            } else if (strcmp(k, "thumbnails_per_row") == 0) {
                config.thumbnails_per_row = atoi(v);
            }
        }
    }
    
    fclose(f);
    return config;
}

void config_print(const Config *config) {
    printf("Configuration:\n");
    printf("  wallpaper_dir: %s\n", config->wallpaper_dir);
    printf("  feh_command: %s\n", config->feh_command);
    printf("  palette_script: %s\n", config->palette_script);
    printf("  thumbnail_size: %dx%d\n", config->thumbnail_width, config->thumbnail_height);
    printf("  window_size: %dx%d\n", config->window_width, config->window_height);
    printf("  use_shaders: %s\n", config->use_shaders ? "true" : "false");
    printf("  thumbnails_per_row: %d\n", config->thumbnails_per_row);
}
EOF

cat > src/thumbnails.c << 'EOF'
#include "thumbnails.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <SDL2/SDL_image.h>

static bool is_image_file(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext) return false;
    
    return (strcasecmp(ext, ".jpg") == 0 ||
            strcasecmp(ext, ".jpeg") == 0 ||
            strcasecmp(ext, ".png") == 0 ||
            strcasecmp(ext, ".bmp") == 0);
}

WallpaperList wallpaper_list_scan(const char *dir) {
    WallpaperList list = {0};
    list.capacity = 32;
    list.items = malloc(sizeof(Wallpaper) * list.capacity);
    
    DIR *d = opendir(dir);
    if (!d) {
        fprintf(stderr, "Failed to open directory: %s\n", dir);
        return list;
    }
    
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (!is_image_file(entry->d_name)) continue;
        
        // Expand capacity if needed
        if (list.count >= list.capacity) {
            list.capacity *= 2;
            list.items = realloc(list.items, sizeof(Wallpaper) * list.capacity);
        }
        
        Wallpaper *wp = &list.items[list.count++];
        
        // Full path
        wp->path = malloc(strlen(dir) + strlen(entry->d_name) + 2);
        sprintf(wp->path, "%s/%s", dir, entry->d_name);
        
        // Filename only
        wp->name = strdup(entry->d_name);
        wp->thumb = NULL;
    }
    
    closedir(d);
    return list;
}

void wallpaper_list_generate_thumbnails(WallpaperList *list, const Config *config) {
    for (int i = 0; i < list->count; i++) {
        list->items[i].thumb = thumbnail_load_or_cache(
            list->items[i].path,
            config->thumbnail_width,
            config->thumbnail_height
        );
    }
}

SDL_Surface* thumbnail_load_or_cache(const char *path, int width, int height) {
    // TODO: Implement caching to ~/.cache/vista/
    // For now, just load and scale
    
    SDL_Surface *original = IMG_Load(path);
    if (!original) {
        fprintf(stderr, "Failed to load image %s: %s\n", path, IMG_GetError());
        return NULL;
    }
    
    // Create scaled surface
    SDL_Surface *thumb = SDL_CreateRGBSurface(0, width, height, 32,
                                              0x00FF0000, 0x0000FF00,
                                              0x000000FF, 0xFF000000);
    
    SDL_Rect dest = {0, 0, width, height};
    SDL_BlitScaled(original, NULL, thumb, &dest);
    
    SDL_FreeSurface(original);
    return thumb;
}

void wallpaper_list_free(WallpaperList *list) {
    for (int i = 0; i < list->count; i++) {
        free(list->items[i].path);
        free(list->items[i].name);
        if (list->items[i].thumb) {
            SDL_FreeSurface(list->items[i].thumb);
        }
    }
    free(list->items);
}
EOF

cat > src/renderer.c << 'EOF'
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
EOF

cat > src/wallpaper.c << 'EOF'
#include "wallpaper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int wallpaper_apply(const char *path, const Config *config) {
    pid_t pid = fork();
    
    if (pid == 0) {
        // Child process
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "%s \"%s\"", config->feh_command, path);
        execlp("sh", "sh", "-c", cmd, NULL);
        exit(1);
    } else if (pid > 0) {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) && WEXITSTATUS(status) == 0 ? 0 : -1;
    }
    
    return -1;
}

int wallpaper_generate_palette(const char *wallpaper_path, const Config *config) {
    if (strlen(config->palette_script) == 0) {
        return 0; // No script configured
    }
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // Child process
        execlp("sh", "sh", "-c", config->palette_script, wallpaper_path, NULL);
        exit(1);
    } else if (pid > 0) {
        // Parent - don't wait, let it run async
        return 0;
    }
    
    return -1;
}
EOF

cat > src/shader.c << 'EOF'
#ifdef USE_SHADERS

#include "shader.h"
#include <stdio.h>
#include <stdlib.h>

GLRenderer* gl_renderer_init(const Config *config) {
    // TODO: Implement OpenGL renderer
    fprintf(stderr, "OpenGL renderer not yet implemented\n");
    return NULL;
}

void gl_renderer_draw_frame(GLRenderer *r, const WallpaperList *list, int selected_index) {
    // TODO: Implement GL rendering
}

GLuint shader_load(const char *path, GLenum type) {
    // TODO: Implement shader loading
    return 0;
}

void gl_renderer_cleanup(GLRenderer *r) {
    // TODO: Implement cleanup
}

#endif /* USE_SHADERS */
EOF

# Create shader files
echo -e "${BLUE}Creating shader files...${NC}"

cat > shaders/vertex.glsl << 'EOF'
#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 projection;
uniform mat4 model;

void main() {
    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
EOF

cat > shaders/fragment.glsl << 'EOF'
#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D texture1;
uniform float selected;
uniform float time;

void main() {
    vec4 texColor = texture(texture1, TexCoord);
    
    if (selected > 0.5) {
        // Highlight selected thumbnail
        float glow = 0.2 + 0.1 * sin(time * 3.0);
        texColor.rgb += vec3(glow);
    }
    
    FragColor = texColor;
}
EOF

# Create example config
echo -e "${BLUE}Creating example config...${NC}"

cat > config/vista.conf.example << 'EOF'
# vista wallpaper switcher configuration
# Copy to ~/.config/vista/config and customize

# Directory containing wallpapers
wallpaper_dir = ~/Pictures/Wallpapers

# Command to set wallpaper (feh, nitrogen, etc.)
feh_command = feh --bg-scale

# Script to generate color palette (optional)
# Leave empty to disable
palette_script = 

# Thumbnail dimensions (pixels)
thumbnail_width = 200
thumbnail_height = 150

# Window dimensions (pixels)
window_width = 1200
window_height = 300

# Enable OpenGL shader rendering (true/false)
# Requires vista to be built with USE_SHADERS=ON
use_shaders = false

# Number of thumbnails per row
thumbnails_per_row = 5
EOF

# Create documentation generation script
echo -e "${BLUE}Creating documentation script...${NC}"

cat > docs/generate_docs.sh << 'EOF'
#!/bin/bash
# Generate Doxygen documentation for vista

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Check if Doxygen is installed
if ! command -v doxygen &> /dev/null; then
    echo "Error: Doxygen is not installed"
    echo "Install with: sudo pacman -S doxygen  (Arch)"
    echo "           or: sudo apt install doxygen (Debian/Ubuntu)"
    exit 1
fi

# Create Doxyfile if it doesn't exist
if [ ! -f "$SCRIPT_DIR/Doxyfile" ]; then
    echo "Creating Doxyfile..."
    cd "$SCRIPT_DIR"
    doxygen -g Doxyfile
    
    # Customize Doxyfile
    sed -i 's/PROJECT_NAME           = "My Project"/PROJECT_NAME           = "vista"/' Doxyfile
    sed -i 's/PROJECT_BRIEF          =/PROJECT_BRIEF          = "SDL2-based wallpaper switcher"/' Doxyfile
    sed -i 's/PROJECT_NUMBER         =/PROJECT_NUMBER         = 1.0.0/' Doxyfile
    sed -i 's/OUTPUT_DIRECTORY       =/OUTPUT_DIRECTORY       = ./' Doxyfile
    sed -i 's/INPUT                  =/INPUT                  = ..\/src ..\/include/' Doxyfile
    sed -i 's/RECURSIVE              = NO/RECURSIVE              = YES/' Doxyfile
    sed -i 's/EXTRACT_ALL            = NO/EXTRACT_ALL            = YES/' Doxyfile
    sed -i 's/GENERATE_LATEX         = YES/GENERATE_LATEX         = NO/' Doxyfile
    sed -i 's/HAVE_DOT               = NO/HAVE_DOT               = YES/' Doxyfile
    sed -i 's/CALL_GRAPH             = NO/CALL_GRAPH             = YES/' Doxyfile
    sed -i 's/CALLER_GRAPH           = NO/CALLER_GRAPH           = YES/' Doxyfile
fi

# Generate documentation
echo "Generating documentation..."
cd "$SCRIPT_DIR"
doxygen Doxyfile

if [ $? -eq 0 ]; then
    echo "Documentation generated successfully!"
    echo "Open: $SCRIPT_DIR/html/index.html"
else
    echo "Error generating documentation"
    exit 1
fi
EOF

chmod +x docs/generate_docs.sh

# Create build helper script
echo -e "${BLUE}Creating build script...${NC}"

cat > build.sh << 'EOF'
#!/bin/bash
# Build script for vista

set -e

BUILD_TYPE="${1:-Release}"
USE_SHADERS="${2:-OFF}"

echo "Building vista..."
echo "  Build type: $BUILD_TYPE"
echo "  Shaders: $USE_SHADERS"

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DUSE_SHADERS="$USE_SHADERS" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build
cmake --build . -j$(nproc)

echo ""
echo "Build complete! Binary: ./build/vista"
echo ""
echo "To install system-wide:"
echo "  cd build && sudo cmake --install ."
echo ""
echo "To run from build directory:"
echo "  ./build/vista"
EOF

chmod +x build.sh

# Create .gitignore
echo -e "${BLUE}Creating .gitignore...${NC}"

cat > .gitignore << 'EOF'
# Build artifacts
build/
*.o
*.a
*.so
*.dylib
vista

# CMake
CMakeCache.txt
CMakeFiles/
cmake_install.cmake
install_manifest.txt
compile_commands.json

# Cache
.cache/

# Documentation
docs/html/
docs/latex/
docs/Doxyfile

# IDE
.vscode/
.idea/
*.swp
*.swo
*~

# OS
.DS_Store
Thumbs.db
EOF

# Create install script
echo -e "${BLUE}Creating install script...${NC}"

cat > install.sh << 'EOF'
#!/bin/bash
# Install vista system-wide

set -e

if [ "$EUID" -eq 0 ]; then 
    echo "Don't run this as root - it will prompt for sudo when needed"
    exit 1
fi

# Check if vista is built
if [ ! -f "build/vista" ]; then
    echo "vista is not built yet. Building now..."
    ./build.sh Release OFF
fi

echo "Installing vista..."

# Install binary
sudo install -Dm755 build/vista /usr/local/bin/vista

# Install example config
sudo install -Dm644 config/vista.conf.example /usr/share/vista/vista.conf.example

# Install shaders
sudo mkdir -p /usr/share/vista/shaders
sudo install -Dm644 shaders/vertex.glsl /usr/share/vista/shaders/vertex.glsl
sudo install -Dm644 shaders/fragment.glsl /usr/share/vista/shaders/fragment.glsl

# Create user config directory
mkdir -p ~/.config/vista

# Copy example config if user config doesn't exist
if [ ! -f ~/.config/vista/config ]; then
    echo "Creating default config at ~/.config/vista/config"
    cp config/vista.conf.example ~/.config/vista/config
    echo "Please edit ~/.config/vista/config to set your wallpaper directory"
fi

# Create cache directory
mkdir -p ~/.cache/vista

echo ""
echo "✓ Installation complete!"
echo ""
echo "Next steps:"
echo "  1. Edit ~/.config/vista/config to set your wallpaper directory"
echo "  2. Run 'vista' to launch the wallpaper switcher"
echo "  3. Add to your i3 config:"
echo "     bindsym \$mod+w exec vista"
echo ""
EOF

chmod +x install.sh

# Create uninstall script
echo -e "${BLUE}Creating uninstall script...${NC}"

cat > uninstall.sh << 'EOF'
#!/bin/bash
# Uninstall vista

set -e

if [ "$EUID" -eq 0 ]; then 
    echo "Don't run this as root - it will prompt for sudo when needed"
    exit 1
fi

echo "Uninstalling vista..."

# Remove binary
sudo rm -f /usr/local/bin/vista

# Remove shared files
sudo rm -rf /usr/share/vista

echo ""
echo "✓ Vista uninstalled"
echo ""
echo "User configuration and cache preserved at:"
echo "  ~/.config/vista/"
echo "  ~/.cache/vista/"
echo ""
echo "To remove these as well:"
echo "  rm -rf ~/.config/vista ~/.cache/vista"
echo ""
EOF

chmod +x uninstall.sh

# Create basic man page
echo -e "${BLUE}Creating man page...${NC}"

cat > docs/vista.1 << 'EOF'
.TH VISTA 1 "October 2025" "vista 1.0.0" "User Commands"
.SH NAME
vista \- SDL2-based wallpaper switcher
.SH SYNOPSIS
.B vista
[\fIOPTIONS\fR]
.SH DESCRIPTION
.B vista
is a lightweight, keyboard-driven wallpaper switcher with thumbnail previews.
It allows you to quickly browse and select wallpapers using arrow keys.
.SH OPTIONS
.TP
.BR \-c ", " \-\-config " " \fIPATH\fR
Use alternative config file instead of ~/.config/vista/config
.TP
.BR \-h ", " \-\-help
Display help message and exit
.TP
.BR \-v ", " \-\-version
Display version information and exit
.SH USAGE
.TP
.B Left Arrow
Select previous wallpaper
.TP
.B Right Arrow
Select next wallpaper
.TP
.B Enter
Apply selected wallpaper and exit
.TP
.B Escape
Exit without applying wallpaper
.SH FILES
.TP
.I ~/.config/vista/config
User configuration file
.TP
.I ~/.cache/vista/
Thumbnail cache directory
.TP
.I /usr/share/vista/vista.conf.example
Example configuration file
.SH CONFIGURATION
Configuration file uses key=value format. See /usr/share/vista/vista.conf.example
for available options.
.SH EXAMPLES
.TP
Launch with default config:
.B vista
.TP
Launch with custom config:
.B vista \-c /path/to/config
.TP
Add to i3 config:
.B bindsym $mod+w exec vista
.SH AUTHORS
Written for the suckless community.
.SH SEE ALSO
.BR feh (1),
.BR nitrogen (1),
.BR SDL2 (3)
EOF

# Create development helper Makefile
echo -e "${BLUE}Creating Makefile wrapper...${NC}"

cat > Makefile << 'EOF'
# Makefile wrapper for CMake-based vista project

.PHONY: all build clean install uninstall debug release shaders docs help

all: release

# Release build (default)
release:
	@./build.sh Release OFF

# Debug build
debug:
	@./build.sh Debug OFF

# Build with shader support
shaders:
	@./build.sh Release ON

# Debug build with shaders
debug-shaders:
	@./build.sh Debug ON

# Generate documentation
docs:
	@cd docs && ./generate_docs.sh

# Clean build artifacts
clean:
	@rm -rf build/
	@echo "Build directory cleaned"

# Install system-wide
install:
	@./install.sh

# Uninstall
uninstall:
	@./uninstall.sh

# Run from build directory
run: release
	@./build/vista

# Help
help:
	@echo "vista - Makefile targets:"
	@echo ""
	@echo "  make                - Build release version (default)"
	@echo "  make release        - Build release version"
	@echo "  make debug          - Build debug version"
	@echo "  make shaders        - Build release with OpenGL shaders"
	@echo "  make debug-shaders  - Build debug with OpenGL shaders"
	@echo "  make docs           - Generate Doxygen documentation"
	@echo "  make clean          - Remove build artifacts"
	@echo "  make install        - Install system-wide"
	@echo "  make uninstall      - Remove installation"
	@echo "  make run            - Build and run vista"
	@echo "  make help           - Show this help"
	@echo ""
	@echo "Dependencies:"
	@echo "  - SDL2, SDL2_image"
	@echo "  - For shaders: OpenGL, GLEW"
	@echo "  - For docs: Doxygen"
	@echo ""
	@echo "Quick start:"
	@echo "  1. make              # Build"
	@echo "  2. make install      # Install system-wide"
	@echo "  3. vista             # Run!"
EOF

# Create contributing guide
echo -e "${BLUE}Creating CONTRIBUTING.md...${NC}"

cat > CONTRIBUTING.md << 'EOF'
# Contributing to vista

Thanks for your interest in contributing to vista!

## Building

```bash
# Quick build
make

# Or with full options
./build.sh Release OFF

# With shaders
make shaders
```

## Code Style

- Follow existing code style (K&R-ish)
- Use 4 spaces for indentation
- Comment your code, especially complex algorithms
- Keep functions focused and under ~50 lines when possible
- Use meaningful variable names

## Documentation

- Add Doxygen comments to new functions
- Update man page if adding new options
- Regenerate docs: `make docs`

## Testing

Before submitting:
1. Build without warnings: `make debug`
2. Test on your wallpaper directory
3. Verify thumbnail caching works
4. Test with and without config file
5. Check memory leaks: `valgrind ./build/vista`

## Pull Requests

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit PR with clear description

## Feature Ideas

- Grid view mode
- Favorites/bookmarks
- Random wallpaper selection
- Multi-monitor support
- Video wallpaper preview
- Wayland support

## License

By contributing, you agree your code will be licensed under the same license as vista.
EOF

# Create TODO list
echo -e "${BLUE}Creating TODO.md...${NC}"

cat > TODO.md << 'EOF'
# vista TODO

## Core Features
- [ ] Implement thumbnail caching (MD5 hashing)
- [ ] Add smooth scrolling animation
- [ ] Grid view mode
- [ ] Search/filter by filename
- [ ] Favorites system

## Shader Support
- [ ] Complete OpenGL renderer implementation
- [ ] Smooth fade transitions
- [ ] Blur effect for non-selected thumbnails
- [ ] Customizable shader effects

## Configuration
- [ ] XDG base directory support
- [ ] Multiple wallpaper directories
- [ ] Per-monitor wallpaper settings
- [ ] Theme/color scheme options

## User Experience
- [ ] Loading indicator for large directories
- [ ] Keyboard shortcuts help overlay
- [ ] Mouse support (click to select)
- [ ] Preview fullscreen wallpaper
- [ ] Remember last selected wallpaper

## Platform Support
- [ ] Wayland backend
- [ ] Multi-monitor support
- [ ] Different wallpaper setters (nitrogen, xwallpaper, etc.)

## Performance
- [ ] Lazy loading of thumbnails
- [ ] Background thumbnail generation
- [ ] Memory optimization for large collections
- [ ] GPU-accelerated scaling

## Documentation
- [ ] Full API documentation
- [ ] User guide
- [ ] Configuration examples
- [ ] Video tutorial

## Build System
- [ ] Package for AUR
- [ ] Debian package
- [ ] Flatpak support
- [ ] CI/CD pipeline
EOF

# Success message
echo ""
echo -e "${GREEN}✓ Vista project setup complete!${NC}"
echo ""
echo "Project structure:"
echo "  src/        - Source files"
echo "  include/    - Header files"
echo "  shaders/    - GLSL shaders"
echo "  config/     - Example configuration"
echo "  docs/       - Documentation"
echo "  build/      - Build output (run make first)"
echo ""
echo "Quick start:"
echo "  1. Install dependencies:"
echo "     ${BLUE}sudo pacman -S sdl2 sdl2_image cmake${NC}  (Arch)"
echo "     ${BLUE}sudo apt install libsdl2-dev libsdl2-image-dev cmake${NC}  (Debian/Ubuntu)"
echo ""
echo "  2. Build:"
echo "     ${BLUE}make${NC}"
echo ""
echo "  3. Run:"
echo "     ${BLUE}./build/vista${NC}"
echo ""
echo "  4. Install system-wide:"
echo "     ${BLUE}make install${NC}"
echo ""
echo "Optional - enable shaders:"
echo "  ${BLUE}make shaders${NC}  (requires OpenGL and GLEW)"
echo ""
echo "Generate documentation:"
echo "  ${BLUE}make docs${NC}  (requires Doxygen)"
echo ""
echo "Happy coding! 🎨"
