#define _GNU_SOURCE
#include "thumbnails.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <openssl/md5.h>
#include <SDL2/SDL_image.h>

static bool is_image_file(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext) return false;
    
    return (strcasecmp(ext, ".jpg") == 0 ||
            strcasecmp(ext, ".jpeg") == 0 ||
            strcasecmp(ext, ".png") == 0 ||
            strcasecmp(ext, ".bmp") == 0);
}

static void get_cache_dir(char *buffer, size_t size) {
    const char *xdg_cache = getenv("XDG_CACHE_HOME");
    if (xdg_cache) {
        snprintf(buffer, size, "%s/vista", xdg_cache);
    } else {
        const char *home = getenv("HOME");
        if (!home) {
            struct passwd *pw = getpwuid(getuid());
            home = pw->pw_dir;
        }
        snprintf(buffer, size, "%s/.cache/vista", home);
    }
    
    // Create cache directory if it doesn't exist
    mkdir(buffer, 0755);
}

static void compute_md5(const char *path, char *output) {
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, path, strlen(path));
    MD5_Final(digest, &ctx);
    
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(&output[i*2], "%02x", digest[i]);
    }
    output[MD5_DIGEST_LENGTH * 2] = '\0';
}

WallpaperList wallpaper_list_scan(const char *dir) {
    WallpaperList list = {0};
    list.capacity = 32;
    list.items = malloc(sizeof(Wallpaper) * list.capacity);
    list.search_query[0] = '\0';
    list.filtered_indices = NULL;
    list.filtered_count = 0;
    list.show_favorites_only = false;
    
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
        wp->is_favorite = false;
    }
    
    closedir(d);
    
    // Load favorites after scanning
    wallpaper_list_load_favorites(&list);
    
    return list;
}

void wallpaper_list_generate_thumbnails(WallpaperList *list, const Config *config) {
    for (int i = 0; i < list->count; i++) {
        list->items[i].thumb = thumbnail_load_or_cache(
            list->items[i].path,
            config->thumbnail_width,
            config->thumbnail_height
        );
        printf("Generated thumbnail for %s\n", list->items[i].name);
    }
}

SDL_Surface* thumbnail_load_or_cache(const char *path, int width, int height) {
    char cache_dir[512];
    char md5[MD5_DIGEST_LENGTH * 2 + 1];
    char cache_path[768];
    
    get_cache_dir(cache_dir, sizeof(cache_dir));
    compute_md5(path, md5);
    snprintf(cache_path, sizeof(cache_path), "%s/%s_%dx%d.png", cache_dir, md5, width, height);
    
    // Try to load from cache
    SDL_Surface *thumb = IMG_Load(cache_path);
    if (thumb) {
        return thumb;
    }
    
    // Cache miss - load original and create thumbnail
    SDL_Surface *original = IMG_Load(path);
    if (!original) {
        fprintf(stderr, "Failed to load image %s: %s\n", path, IMG_GetError());
        return NULL;
    }
    
    // Create scaled surface
    thumb = SDL_CreateRGBSurface(0, width, height, 32,
                                              0x00FF0000, 0x0000FF00,
                                              0x000000FF, 0xFF000000);
    
    SDL_Rect dest = {0, 0, width, height};
    SDL_BlitScaled(original, NULL, thumb, &dest);
    
    // Save to cache
    IMG_SavePNG(thumb, cache_path);
    
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
    if (list->filtered_indices) {
        free(list->filtered_indices);
    }
}

void wallpaper_list_filter(WallpaperList *list, const char *query) {
    strncpy(list->search_query, query, sizeof(list->search_query) - 1);
    list->search_query[sizeof(list->search_query) - 1] = '\0';
    
    if (list->filtered_indices) {
        free(list->filtered_indices);
    }
    
    list->filtered_indices = malloc(sizeof(int) * list->count);
    list->filtered_count = 0;
    
    for (int i = 0; i < list->count; i++) {
        bool name_matches = strcasestr(list->items[i].name, query) != NULL;
        bool favorites_ok = !list->show_favorites_only || list->items[i].is_favorite;
        
        if (name_matches && favorites_ok) {
            list->filtered_indices[list->filtered_count++] = i;
        }
    }
}

void wallpaper_list_clear_filter(WallpaperList *list) {
    list->search_query[0] = '\0';
    if (list->filtered_indices) {
        free(list->filtered_indices);
        list->filtered_indices = NULL;
    }
    list->filtered_count = 0;
}

Wallpaper* wallpaper_list_get(WallpaperList *list, int index) {
    if (list->filtered_count > 0) {
        if (index >= 0 && index < list->filtered_count) {
            return &list->items[list->filtered_indices[index]];
        }
    } else if (list->show_favorites_only) {
        int fav_count = 0;
        for (int i = 0; i < list->count; i++) {
            if (list->items[i].is_favorite) {
                if (fav_count == index) {
                    return &list->items[i];
                }
                fav_count++;
            }
        }
    } else {
        if (index >= 0 && index < list->count) {
            return &list->items[index];
        }
    }
    return NULL;
}

int wallpaper_list_visible_count(const WallpaperList *list) {
    if (list->filtered_count > 0) {
        return list->filtered_count;
    }
    
    if (list->show_favorites_only) {
        int count = 0;
        for (int i = 0; i < list->count; i++) {
            if (list->items[i].is_favorite) count++;
        }
        return count;
    }
    
    return list->count;
}

static void get_favorites_path(char *buffer, size_t size) {
    const char *xdg_data = getenv("XDG_DATA_HOME");
    if (xdg_data) {
        snprintf(buffer, size, "%s/vista/favorites.txt", xdg_data);
    } else {
        const char *home = getenv("HOME");
        if (!home) {
            struct passwd *pw = getpwuid(getuid());
            home = pw->pw_dir;
        }
        snprintf(buffer, size, "%s/.local/share/vista/favorites.txt", home);
    }
    
    // Create directory if it doesn't exist
    char dir_path[512];
    strncpy(dir_path, buffer, sizeof(dir_path) - 1);
    char *last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';
        mkdir(dir_path, 0755);
    }
}

void wallpaper_toggle_favorite(WallpaperList *list, int index) {
    Wallpaper *wp = wallpaper_list_get(list, index);
    if (wp) {
        wp->is_favorite = !wp->is_favorite;
        wallpaper_list_save_favorites(list);
        printf("%s %s\n", wp->is_favorite ? "Added to favorites:" : "Removed from favorites:", wp->name);
    }
}

void wallpaper_list_toggle_favorites_filter(WallpaperList *list) {
    list->show_favorites_only = !list->show_favorites_only;
    
    // Rebuild filter
    if (list->show_favorites_only || strlen(list->search_query) > 0) {
        if (list->filtered_indices) {
            free(list->filtered_indices);
        }
        
        list->filtered_indices = malloc(sizeof(int) * list->count);
        list->filtered_count = 0;
        
        for (int i = 0; i < list->count; i++) {
            bool name_matches = strlen(list->search_query) == 0 || 
                                strcasestr(list->items[i].name, list->search_query) != NULL;
            bool favorites_ok = !list->show_favorites_only || list->items[i].is_favorite;
            
            if (name_matches && favorites_ok) {
                list->filtered_indices[list->filtered_count++] = i;
            }
        }
    } else {
        wallpaper_list_clear_filter(list);
    }
}

void wallpaper_list_load_favorites(WallpaperList *list) {
    char path[512];
    get_favorites_path(path, sizeof(path));
    
    FILE *f = fopen(path, "r");
    if (!f) return;
    
    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        
        // Find matching wallpaper
        for (int i = 0; i < list->count; i++) {
            if (strcmp(list->items[i].path, line) == 0) {
                list->items[i].is_favorite = true;
                break;
            }
        }
    }
    
    fclose(f);
}

void wallpaper_list_save_favorites(const WallpaperList *list) {
    char path[512];
    get_favorites_path(path, sizeof(path));
    
    FILE *f = fopen(path, "w");
    if (!f) {
        fprintf(stderr, "Failed to save favorites to %s\n", path);
        return;
    }
    
    for (int i = 0; i < list->count; i++) {
        if (list->items[i].is_favorite) {
            fprintf(f, "%s\n", list->items[i].path);
        }
    }
    
    fclose(f);
}

WallpaperList wallpaper_list_scan_multiple(const Config *config) {
    WallpaperList list = wallpaper_list_scan(config->wallpaper_dir);
    
    // Scan additional directories
    for (int i = 0; i < config->wallpaper_dirs_count; i++) {
        DIR *d = opendir(config->wallpaper_dirs[i]);
        if (!d) {
            fprintf(stderr, "Warning: Failed to open directory: %s\n", config->wallpaper_dirs[i]);
            continue;
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
            wp->path = malloc(strlen(config->wallpaper_dirs[i]) + strlen(entry->d_name) + 2);
            sprintf(wp->path, "%s/%s", config->wallpaper_dirs[i], entry->d_name);
            
            // Filename only
            wp->name = strdup(entry->d_name);
            wp->thumb = NULL;
            wp->is_favorite = false;
        }
        
        closedir(d);
    }
    
    // Load favorites for all wallpapers
    wallpaper_list_load_favorites(&list);
    
    return list;
}
