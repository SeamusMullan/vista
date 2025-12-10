/**
 * @file openrgb.c
 * @brief OpenRGB CLI integration for peripheral color control
 */

#include "openrgb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

bool openrgb_is_available(void) {
    // Check if 'openrgb' command exists in PATH
    int result = system("which openrgb >/dev/null 2>&1");
    return (result == 0);
}

int openrgb_set_color_cli(RGBColor color, const char *mode) {
    return openrgb_set_color_cli_brightness(color, mode, -1);
}

int openrgb_set_color_cli_brightness(RGBColor color, const char *mode, int brightness) {
    char cmd[512];
    char hex_color[7];

    // Convert RGB to hex string
    color_rgb_to_hex(color, hex_color);

    // Build OpenRGB command
    // Format: openrgb --color RRGGBB --mode MODE [--brightness N]
    int offset = snprintf(cmd, sizeof(cmd),
                          "openrgb --color %s", hex_color);

    // Add mode if specified
    if (mode && strlen(mode) > 0) {
        offset += snprintf(cmd + offset, sizeof(cmd) - offset,
                          " --mode %s", mode);
    }

    // Add brightness if specified (0-100)
    if (brightness >= 0 && brightness <= 100) {
        offset += snprintf(cmd + offset, sizeof(cmd) - offset,
                          " --brightness %d", brightness);
    }

    // Run in background, suppress output
    snprintf(cmd + offset, sizeof(cmd) - offset,
             " >/dev/null 2>&1 &");

    printf("Running OpenRGB command: openrgb --color %s --mode %s\n",
           hex_color, mode ? mode : "(default)");

    int result = system(cmd);

    if (result != 0) {
        fprintf(stderr, "Warning: OpenRGB command failed (exit code: %d)\n", result);
        return -1;
    }

    return 0;
}

int openrgb_apply_from_config(const char *wallpaper_path, const Config *config) {
    if (!config->use_openrgb) {
        return 0;
    }

    // Check if OpenRGB is available
    if (!openrgb_is_available()) {
        fprintf(stderr, "Warning: OpenRGB not found in PATH. Skipping peripheral color update.\n");
        fprintf(stderr, "         Install OpenRGB from https://openrgb.org/\n");
        return -1;
    }

    // Parse color source type
    ColorSourceType source = color_source_parse_type(config->openrgb_color_source);

    // Get color from source
    RGBColor color = color_source_get_primary(source, wallpaper_path, config);

    printf("OpenRGB: Setting peripherals to color #%02X%02X%02X from source '%s'\n",
           color.r, color.g, color.b, config->openrgb_color_source);

    // Apply color with configured mode and brightness
    const char *mode = (strlen(config->openrgb_mode) > 0) ? config->openrgb_mode : "static";
    int brightness = config->openrgb_brightness;

    return openrgb_set_color_cli_brightness(color, mode, brightness);
}
