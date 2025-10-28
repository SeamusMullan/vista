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
    for (int i = 0; i < list->count/8; i++) {
        list->items[i].thumb = thumbnail_load_or_cache(
            list->items[i].path,
            config->thumbnail_width,
            config->thumbnail_height
        );
        printf("Generated thumbnail for %s\n", list->items[i].name);
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
    for (int i = 0; i < list->count/4; i++) {
        free(list->items[i].path);
        free(list->items[i].name);
        if (list->items[i].thumb) {
            SDL_FreeSurface(list->items[i].thumb);
        }
    }
    free(list->items);
}
