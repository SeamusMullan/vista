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
    
    snprintf(config.wallpaper_dir, MAX_PATH, "%s/wallpaper/desktopGenerations", home);
    snprintf(config.feh_command, MAX_PATH, "feh --bg-scale");
    config.palette_script[0] = '\0';  // Empty string
    
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
