/**
 * @file color_source.c
 * @brief Color extraction from various sources for OpenRGB integration
 */

#include "color_source.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <unistd.h>
#include <pwd.h>

/**
 * @brief Get XDG cache directory or fallback to ~/.cache
 */
static void get_cache_dir(char *buffer, size_t size) {
    const char *xdg_cache = getenv("XDG_CACHE_HOME");
    if (xdg_cache) {
        snprintf(buffer, size, "%s", xdg_cache);
    } else {
        const char *home = getenv("HOME");
        if (!home) {
            struct passwd *pw = getpwuid(getuid());
            home = pw->pw_dir;
        }
        snprintf(buffer, size, "%s/.cache", home);
    }
}

/**
 * @brief Strip leading/trailing whitespace and # from hex color
 */
static void strip_color_string(char *color) {
    // Remove leading whitespace and #
    char *src = color;
    while (*src && (isspace(*src) || *src == '#')) {
        src++;
    }

    // Move to beginning
    if (src != color) {
        memmove(color, src, strlen(src) + 1);
    }

    // Remove trailing whitespace
    size_t len = strlen(color);
    while (len > 0 && isspace(color[len - 1])) {
        color[--len] = '\0';
    }
}

ColorSourceType color_source_parse_type(const char *source_str) {
    if (!source_str || strlen(source_str) == 0) {
        return COLOR_SOURCE_WAL;
    }

    if (strcasecmp(source_str, "wal") == 0) {
        return COLOR_SOURCE_WAL;
    } else if (strcasecmp(source_str, "script") == 0) {
        return COLOR_SOURCE_SCRIPT;
    } else if (strcasecmp(source_str, "static") == 0) {
        return COLOR_SOURCE_STATIC;
    }

    fprintf(stderr, "Warning: Unknown color source '%s', defaulting to 'wal'\n", source_str);
    return COLOR_SOURCE_WAL;
}

RGBColor color_hex_to_rgb(const char *hex) {
    RGBColor color = {255, 255, 255}; // Default to white on error

    if (!hex) {
        return color;
    }

    // Make a mutable copy for stripping
    char hex_copy[32];
    strncpy(hex_copy, hex, sizeof(hex_copy) - 1);
    hex_copy[sizeof(hex_copy) - 1] = '\0';
    strip_color_string(hex_copy);

    // Parse hex string
    if (strlen(hex_copy) == 6) {
        unsigned int r, g, b;
        if (sscanf(hex_copy, "%02x%02x%02x", &r, &g, &b) == 3) {
            color.r = r;
            color.g = g;
            color.b = b;
        }
    } else {
        fprintf(stderr, "Warning: Invalid hex color format: %s\n", hex);
    }

    return color;
}

void color_rgb_to_hex(RGBColor color, char *buffer) {
    snprintf(buffer, 7, "%02X%02X%02X", color.r, color.g, color.b);
}

RGBColor color_source_read_wal(void) {
    char cache_dir[512];
    char colors_path[768];
    RGBColor color = {255, 255, 255}; // Default white

    get_cache_dir(cache_dir, sizeof(cache_dir));
    snprintf(colors_path, sizeof(colors_path), "%s/wal/colors", cache_dir);

    FILE *fp = fopen(colors_path, "r");
    if (!fp) {
        fprintf(stderr, "Warning: Could not read wal colors from %s\n", colors_path);
        fprintf(stderr, "         Make sure pywal has been run at least once.\n");
        return color;
    }

    char line[128];
    int line_num = 0;

    // Read colors from file
    // Line 0: background (usually dark)
    // Line 1-15: accent colors
    // We use line 3 (index 3) which tends to be vibrant
    while (fgets(line, sizeof(line), fp) && line_num < 4) {
        line_num++;
        if (line_num == 4) {
            // Use color at index 3 (line 4 in the file)
            color = color_hex_to_rgb(line);
            break;
        }
    }

    fclose(fp);
    return color;
}

ColorPalette color_source_read_wal_palette(void) {
    char cache_dir[512];
    char colors_path[768];
    ColorPalette palette = {0};

    get_cache_dir(cache_dir, sizeof(cache_dir));
    snprintf(colors_path, sizeof(colors_path), "%s/wal/colors", cache_dir);

    FILE *fp = fopen(colors_path, "r");
    if (!fp) {
        fprintf(stderr, "Warning: Could not read wal colors from %s\n", colors_path);
        return palette;
    }

    char line[128];
    while (fgets(line, sizeof(line), fp) && palette.count < 16) {
        palette.colors[palette.count] = color_hex_to_rgb(line);
        palette.count++;
    }

    fclose(fp);
    return palette;
}

/**
 * @brief Run custom script to get color
 */
static RGBColor color_source_run_script(const char *script_path, const char *wallpaper_path) {
    RGBColor color = {255, 255, 255}; // Default white

    if (!script_path || strlen(script_path) == 0) {
        fprintf(stderr, "Warning: No color script configured\n");
        return color;
    }

    // Build command with wallpaper path as argument
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "%s \"%s\" 2>/dev/null", script_path, wallpaper_path);

    // Run script and capture output
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        fprintf(stderr, "Warning: Failed to run color script: %s\n", script_path);
        return color;
    }

    char output[128];
    if (fgets(output, sizeof(output), fp)) {
        color = color_hex_to_rgb(output);
    } else {
        fprintf(stderr, "Warning: Color script produced no output: %s\n", script_path);
    }

    pclose(fp);
    return color;
}

RGBColor color_source_get_primary(ColorSourceType source, const char *wallpaper_path, const Config *config) {
    RGBColor color = {255, 255, 255}; // Default white

    switch (source) {
        case COLOR_SOURCE_WAL:
            color = color_source_read_wal();
            break;

        case COLOR_SOURCE_SCRIPT:
            if (config && strlen(config->openrgb_color_script) > 0) {
                color = color_source_run_script(config->openrgb_color_script, wallpaper_path);
            } else {
                fprintf(stderr, "Warning: COLOR_SOURCE_SCRIPT selected but no script configured\n");
            }
            break;

        case COLOR_SOURCE_STATIC:
            if (config && strlen(config->openrgb_static_color) > 0) {
                color = color_hex_to_rgb(config->openrgb_static_color);
            } else {
                fprintf(stderr, "Warning: COLOR_SOURCE_STATIC selected but no static color configured\n");
            }
            break;

        default:
            fprintf(stderr, "Warning: Unknown color source type\n");
            break;
    }

    return color;
}

ColorPalette color_source_get_palette(ColorSourceType source, const char *wallpaper_path, const Config *config) {
    ColorPalette palette = {0};

    switch (source) {
        case COLOR_SOURCE_WAL:
            palette = color_source_read_wal_palette();
            break;

        case COLOR_SOURCE_SCRIPT:
        case COLOR_SOURCE_STATIC:
            // For script/static, just get primary color and use it as single-color palette
            palette.colors[0] = color_source_get_primary(source, wallpaper_path, config);
            palette.count = 1;
            break;

        default:
            fprintf(stderr, "Warning: Unknown color source type for palette\n");
            break;
    }

    return palette;
}
