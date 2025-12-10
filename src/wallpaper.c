#include "wallpaper.h"
#include "config.h"
#include "openrgb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

/**
 * @brief Run a command synchronously
 */
static int run_command_sync(const char *cmd) {
    pid_t pid = fork();
    
    if (pid == 0) {
        execlp("sh", "sh", "-c", cmd, NULL);
        exit(1);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) && WEXITSTATUS(status) == 0 ? 0 : -1;
    }
    
    return -1;
}

/**
 * @brief Run a command asynchronously
 */
static int run_command_async(const char *cmd) {
    pid_t pid = fork();
    
    if (pid == 0) {
        // Child process: detach from parent by forking again
        pid_t pid2 = fork();
        
        if (pid2 == 0) {
            // Grandchild: this will be adopted by init when parent exits
            // Redirect stdout/stderr to /dev/null to avoid blocking
            freopen("/dev/null", "w", stdout);
            freopen("/dev/null", "w", stderr);
            
            execlp("sh", "sh", "-c", cmd, NULL);
            exit(1);
        }
        
        // First child exits immediately, orphaning the grandchild
        exit(0);
    } else if (pid > 0) {
        // Parent: wait for first child to exit (prevents zombies)
        int status;
        waitpid(pid, &status, 0);
        return 0;
    }
    
    return -1;
}

/**
 * @brief Build feh command with multi-monitor support
 */
static void build_feh_command(char *cmd, size_t cmd_size, const char *path, const Config *config) {
    if (config->use_per_monitor && config->monitors_count > 0) {
        // Multi-monitor setup: apply to each monitor separately
        char temp[1024] = {0};
        int offset = 0;
        
        for (int i = 0; i < config->monitors_count; i++) {
            if (i > 0) {
                offset += snprintf(temp + offset, sizeof(temp) - offset, " ");
            }
            offset += snprintf(temp + offset, sizeof(temp) - offset, 
                             "--bg-fill --output %s \"%s\"", 
                             config->monitors[i], path);
        }
        snprintf(cmd, cmd_size, "feh %s", temp);
    } else {
        // Single/spanning mode
        snprintf(cmd, cmd_size, "%s \"%s\"", config->feh_command, path);
    }
}

int wallpaper_apply(const char *path, const Config *config) {
    char cmd[1024];
    
    // Step 1: Run pywal if enabled (before setting wallpaper)
    // Run synchronously to ensure colors are generated before OpenRGB reads them
    if (config->use_wal) {
        printf("Generating color scheme with pywal...\n");
        // Use wal with -n flag to skip setting wallpaper (we'll do it ourselves)
        // This allows wal to focus on generating colors and updating terminals
        if (strlen(config->wal_options) > 0) {
            snprintf(cmd, sizeof(cmd), "wal -i \"%s\" -n %s", path, config->wal_options);
        } else {
            snprintf(cmd, sizeof(cmd), "wal -i \"%s\" -n", path);
        }
        run_command_sync(cmd);
    }
    
    // Step 2: Set the wallpaper explicitly with our configured method
    printf("Setting wallpaper: %s\n", path);
    config_print(config);

    // Support different wallpaper setters
    if (strstr(config->feh_command, "feh") != NULL) {
        build_feh_command(cmd, sizeof(cmd), path, config);
    } else if (strstr(config->feh_command, "nitrogen") != NULL) {
        snprintf(cmd, sizeof(cmd), "nitrogen --set-scaled \"%s\"", path);
    } else if (strstr(config->feh_command, "xwallpaper") != NULL) {
        snprintf(cmd, sizeof(cmd), "xwallpaper --zoom \"%s\"", path);
    } else if (strstr(config->feh_command, "swaybg") != NULL) {
        snprintf(cmd, sizeof(cmd), "killall swaybg; swaybg -i \"%s\" -m fill &", path);
    } else {
        // Custom command
        snprintf(cmd, sizeof(cmd), "%s \"%s\"", config->feh_command, path);
    }
    
    // Run wallpaper setter asynchronously - no need to wait for completion
    int result = run_command_async(cmd);

    // Step 2.5: Update OpenRGB peripheral colors
    if (config->use_openrgb) {
        printf("Updating OpenRGB peripheral colors...\n");
        openrgb_apply_from_config(path, config);
    }

    // Step 3: Reload i3 if enabled
    if (config->reload_i3) {
        printf("Reloading i3 configuration...\n");
        run_command_async("i3-msg reload");
    }

    // Step 4: Run post command if configured
    if (strlen(config->post_command) > 0) {
        printf("Running post command...\n");
        snprintf(cmd, sizeof(cmd), "%s \"%s\"", config->post_command, path);
        run_command_async(cmd);
    }
    
    return result;
}

int wallpaper_generate_palette(const char *wallpaper_path, const Config *config) {
    if (strlen(config->palette_script) == 0) {
        return 0; // No script configured
    }
    
    printf("Running palette script: %s\n", config->palette_script);
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "%s \"%s\"", config->palette_script, wallpaper_path);
    
    return run_command_async(cmd);
}
