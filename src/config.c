#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>

/* -------------------------------------------------------------------------- */
/*                                Helper Utils                                */
/* -------------------------------------------------------------------------- */

// Safely get home directory
static const char *get_home_dir(void)
{
    const char *home = getenv("HOME");
    if (!home)
    {
        struct passwd *pw = getpwuid(getuid());
        if (pw)
            home = pw->pw_dir;
    }
    return home ? home : "";
}

// Expand '~' to $HOME
static void expand_tilde(const char *input, char *output, size_t size)
{
    if (input[0] == '~')
    {
        const char *home = get_home_dir();
        snprintf(output, size, "%s%s", home, input + 1);
    }
    else
    {
        snprintf(output, size, "%s", input);
    }
}

// Build the path to the config file (XDG or fallback)
static void get_xdg_config_path(char *buffer, size_t size)
{
    const char *xdg_config = getenv("XDG_CONFIG_HOME");
    if (xdg_config)
        snprintf(buffer, size, "%s/vista/vista.conf", xdg_config);
    else
        snprintf(buffer, size, "%s/.config/vista/vista.conf", get_home_dir());
}

/* -------------------------------------------------------------------------- */
/*                             Default Configuration                          */
/* -------------------------------------------------------------------------- */

Config config_default(void)
{
    Config config = {0};

    const char *home = get_home_dir();

    snprintf(config.wallpaper_dir, MAX_PATH, "%s/wallpaper/desktopGenerations", home);
    config.wallpaper_dirs_count = 0;

    snprintf(config.feh_command, MAX_PATH, "feh --bg-scale");
    config.palette_script[0] = '\0';

    config.monitors_count = 0;
    config.use_per_monitor = false;

    config.use_wal = false;
    config.wal_options[0] = '\0';
    config.reload_i3 = false;
    config.post_command[0] = '\0';

    config.thumbnail_width = 200;
    config.thumbnail_height = 150;
    config.window_width = 1200;
    config.window_height = 300;
    config.use_shaders = false;
    config.thumbnails_per_row = 5;
    config.audio_dir[0] = '\0';
    
    // Roulette defaults
    config.roulette_start_duration = 800;
    config.roulette_scroll_duration = 2000;
    config.roulette_slow_duration = 2500;
    config.roulette_show_duration = 1500;
    config.roulette_max_velocity = 80.0f;

    return config;
}

/* -------------------------------------------------------------------------- */
/*                               Parse Function                               */
/* -------------------------------------------------------------------------- */

Config config_parse(const char *path)
{
    Config config = config_default();

    // If no path provided, use XDG config location
    char xdg_path[512];
    if (!path)
    {
        get_xdg_config_path(xdg_path, sizeof(xdg_path));
        path = xdg_path;
    }

    FILE *f = fopen(path, "r");
    if (!f)
        return config; // Silent fail (default config)

    char line[512];
    while (fgets(line, sizeof(line), f))
    {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n')
            continue;

        char key[128], value[384];
        if (sscanf(line, " %127[^=]=%383[^\n]", key, value) == 2)
        {
            // Trim whitespace around key
            char *k = key;
            while (*k == ' ' || *k == '\t')
                k++;
            char *end = k + strlen(k) - 1;
            while (end > k && (*end == ' ' || *end == '\t'))
                *end-- = '\0';

            // Trim whitespace around value
            char *v = value;
            while (*v == ' ' || *v == '\t')
                v++;
            end = v + strlen(v) - 1;
            while (end > v && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n'))
                *end-- = '\0';

            // Strip surrounding quotes
            size_t len = strlen(v);
            if (len >= 2 && ((v[0] == '"' && v[len - 1] == '"') || (v[0] == '\'' && v[len - 1] == '\'')))
            {
                v[len - 1] = '\0';
                v++;
            }

            /* -------------------------- Assign Configuration ------------------------- */

            if (strcmp(k, "wallpaper_dir") == 0)
            {
                expand_tilde(v, config.wallpaper_dir, MAX_PATH);
            }
            else if (strncmp(k, "wallpaper_dir_", 14) == 0)
            {
                if (config.wallpaper_dirs_count < MAX_WALLPAPER_DIRS)
                {
                    expand_tilde(v, config.wallpaper_dirs[config.wallpaper_dirs_count], MAX_PATH);
                    config.wallpaper_dirs_count++;
                }
            }
            else if (strcmp(k, "feh_command") == 0)
            {
                strncpy(config.feh_command, v, MAX_PATH - 1);
            }
            else if (strcmp(k, "palette_script") == 0)
            {
                expand_tilde(v, config.palette_script, MAX_PATH);
                printf("DEBUG: Parsed palette_script = '%s'\n", config.palette_script);
            }
            else if (strncmp(k, "monitor_", 8) == 0)
            {
                if (config.monitors_count < MAX_MONITORS)
                {
                    strncpy(config.monitors[config.monitors_count], v, 63);
                    config.monitors[config.monitors_count][63] = '\0';
                    config.monitors_count++;
                }
            }
            else if (strcmp(k, "use_per_monitor") == 0)
            {
                config.use_per_monitor = (strcmp(v, "true") == 0 || strcmp(v, "1") == 0);
            }
            else if (strcmp(k, "use_wal") == 0)
            {
                config.use_wal = (strcmp(v, "true") == 0 || strcmp(v, "1") == 0);
            }
            else if (strcmp(k, "wal_options") == 0)
            {
                strncpy(config.wal_options, v, sizeof(config.wal_options) - 1);
            }
            else if (strcmp(k, "reload_i3") == 0)
            {
                config.reload_i3 = (strcmp(v, "true") == 0 || strcmp(v, "1") == 0);
            }
            else if (strcmp(k, "post_command") == 0)
            {
                strncpy(config.post_command, v, MAX_COMMAND - 1);
            }
            else if (strcmp(k, "thumbnail_width") == 0)
            {
                config.thumbnail_width = atoi(v);
            }
            else if (strcmp(k, "thumbnail_height") == 0)
            {
                config.thumbnail_height = atoi(v);
            }
            else if (strcmp(k, "window_width") == 0)
            {
                config.window_width = atoi(v);
            }
            else if (strcmp(k, "window_height") == 0)
            {
                config.window_height = atoi(v);
            }
            else if (strcmp(k, "use_shaders") == 0)
            {
                config.use_shaders = (strcmp(v, "true") == 0 || strcmp(v, "1") == 0);
            }
            else if (strcmp(k, "thumbnails_per_row") == 0)
            {
                config.thumbnails_per_row = atoi(v);
            }
            else if (strcmp(k, "audio_dir") == 0)
            {
                expand_tilde(v, config.audio_dir, MAX_PATH);
            }
            else if (strcmp(k, "roulette_start_duration") == 0)
            {
                config.roulette_start_duration = atoi(v);
            }
            else if (strcmp(k, "roulette_scroll_duration") == 0)
            {
                config.roulette_scroll_duration = atoi(v);
            }
            else if (strcmp(k, "roulette_slow_duration") == 0)
            {
                config.roulette_slow_duration = atoi(v);
            }
            else if (strcmp(k, "roulette_show_duration") == 0)
            {
                config.roulette_show_duration = atoi(v);
            }
            else if (strcmp(k, "roulette_max_velocity") == 0)
            {
                config.roulette_max_velocity = atof(v);
            }
        }
    }

    fclose(f);
    return config;
}

/* -------------------------------------------------------------------------- */
/*                              Print Function                                */
/* -------------------------------------------------------------------------- */

void config_print(const Config *config)
{
    printf("Configuration:\n");
    printf("  wallpaper_dir: %s\n", config->wallpaper_dir);
    printf("  feh_command: %s\n", config->feh_command);
    printf("  palette_script: %s\n", config->palette_script);
    printf("  use_wal: %s\n", config->use_wal ? "true" : "false");
    printf("  reload_i3: %s\n", config->reload_i3 ? "true" : "false");

    if (config->monitors_count > 0)
    {
        printf("  monitors: ");
        for (int i = 0; i < config->monitors_count; i++)
        {
            printf("%s%s", config->monitors[i], (i < config->monitors_count - 1) ? ", " : "");
        }
        printf("\n");
        printf("  use_per_monitor: %s\n", config->use_per_monitor ? "true" : "false");
    }

    if (strlen(config->post_command) > 0)
        printf("  post_command: %s\n", config->post_command);

    printf("  thumbnail_size: %dx%d\n", config->thumbnail_width, config->thumbnail_height);
    printf("  window_size: %dx%d\n", config->window_width, config->window_height);
    printf("  use_shaders: %s\n", config->use_shaders ? "true" : "false");
    printf("  thumbnails_per_row: %d\n", config->thumbnails_per_row);
}
