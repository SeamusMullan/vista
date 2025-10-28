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
