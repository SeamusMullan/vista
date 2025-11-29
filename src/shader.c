#ifdef USE_SHADERS

#include "shader.h"
#include "wallpaper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <SDL3/SDL.h>

// Track start time for animations
static Uint64 start_time = 0;

static char* read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Failed to open shader file: %s\n", path);
        return NULL;
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *buffer = malloc(size + 1);
    fread(buffer, 1, size, f);
    buffer[size] = '\0';
    
    fclose(f);
    return buffer;
}

GLuint shader_load(const char *path, GLenum type) {
    char *source = read_file(path);
    if (!source) return 0;
    
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, (const char**)&source, NULL);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, NULL, log);
        fprintf(stderr, "Shader compilation failed (%s): %s\n", path, log);
        free(source);
        return 0;
    }
    
    free(source);
    return shader;
}

GLRenderer* gl_renderer_init(const Config *config) {
    GLRenderer *r = malloc(sizeof(GLRenderer));
    if (!r) return NULL;
    
    // Initialize state
    r->selected_index = 0;
    r->scroll_offset = 0;
    r->target_scroll = 0.0f;
    r->current_scroll = 0.0f;
    r->view_mode = 0;
    r->grid_scroll_y = 0;
    r->target_scroll_y = 0.0f;
    r->current_scroll_y = 0.0f;
    r->search_mode = false;
    r->show_help = false;
    
    // Initialize start time for animations
    start_time = SDL_GetTicks();
    
    // Set OpenGL attributes BEFORE creating window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    
    // CRITICAL: Request alpha channel for transparency
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    
    // Create window with transparency support
    // NOTE: SDL_WINDOW_TRANSPARENT is needed for true compositor transparency
    Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS;
    
    // Try to enable transparency if available (SDL3 feature)
    #ifdef SDL_WINDOW_TRANSPARENT
    window_flags |= SDL_WINDOW_TRANSPARENT;
    #endif
    
    r->window = SDL_CreateWindow(
        "vista - wallpaper switcher (OpenGL)",
        config->window_width,
        config->window_height,
        window_flags
    );
    
    if (!r->window) {
        fprintf(stderr, "Failed to create window: %s\n", SDL_GetError());
        free(r);
        return NULL;
    }
    
    // Position window in center of screen
    SDL_SetWindowPosition(r->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    
    r->gl_context = SDL_GL_CreateContext(r->window);
    if (!r->gl_context) {
        fprintf(stderr, "Failed to create GL context: %s\n", SDL_GetError());
        SDL_DestroyWindow(r->window);
        free(r);
        return NULL;
    }
    
    // Enable VSync
    SDL_GL_SetSwapInterval(1);
    
    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        SDL_GL_DestroyContext(r->gl_context);
        SDL_DestroyWindow(r->window);
        free(r);
        return NULL;
    }
    
    // Load shaders
    GLuint vertex_shader = shader_load("shaders/vertex.glsl", GL_VERTEX_SHADER);
    GLuint fragment_shader = shader_load("shaders/fragment.glsl", GL_FRAGMENT_SHADER);
    
    if (!vertex_shader || !fragment_shader) {
        fprintf(stderr, "Failed to load shaders\n");
        SDL_GL_DestroyContext(r->gl_context);
        SDL_DestroyWindow(r->window);
        free(r);
        return NULL;
    }
    
    r->shader_program = glCreateProgram();
    glAttachShader(r->shader_program, vertex_shader);
    glAttachShader(r->shader_program, fragment_shader);
    glLinkProgram(r->shader_program);
    
    GLint success;
    glGetProgramiv(r->shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(r->shader_program, 512, NULL, log);
        fprintf(stderr, "Shader linking failed: %s\n", log);
    }
    
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    // Setup VAO, VBO for rendering quads
    float vertices[] = {
        // pos      // tex
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f
    };
    
    glGenVertexArrays(1, &r->vao);
    glGenBuffers(1, &r->vbo);
    
    glBindVertexArray(r->vao);
    glBindBuffer(GL_ARRAY_BUFFER, r->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // For additive glow blending on top
    // We'll use standard alpha blending but render glow first
    
    return r;
}

// Helper function to calculate average color from thumbnail
static void calculate_avg_color(SDL_Surface *surf, float *r, float *g, float *b) {
    if (!surf || !surf->pixels) {
        *r = *g = *b = 0.5f;
        return;
    }
    
    unsigned long long sum_r = 0, sum_g = 0, sum_b = 0;
    int sample_count = 0;
    int step = 4;
    
    SDL_LockSurface(surf);
    
    const SDL_PixelFormatDetails *format_details = SDL_GetPixelFormatDetails(surf->format);
    
    for (int y = 0; y < surf->h; y += step) {
        for (int x = 0; x < surf->w; x += step) {
            Uint32 pixel = ((Uint32*)surf->pixels)[y * (surf->pitch / 4) + x];
            Uint8 pr, pg, pb;
            SDL_GetRGB(pixel, format_details, NULL, &pr, &pg, &pb);
            sum_r += pr;
            sum_g += pg;
            sum_b += pb;
            sample_count++;
        }
    }
    
    SDL_UnlockSurface(surf);
    
    if (sample_count > 0) {
        *r = (float)(sum_r / sample_count) / 255.0f;
        *g = (float)(sum_g / sample_count) / 255.0f;
        *b = (float)(sum_b / sample_count) / 255.0f;
    } else {
        *r = *g = *b = 0.5f;
    }
}

// Helper to set up orthographic projection matrix
static void setup_ortho_projection(float *projection, float width, float height) {
    float left = 0.0f, right = width;
    float bottom = height, top = 0.0f;
    float near_val = -1.0f, far_val = 1.0f;
    
    memset(projection, 0, 16 * sizeof(float));
    projection[0] = 2.0f / (right - left);
    projection[5] = 2.0f / (top - bottom);
    projection[10] = -2.0f / (far_val - near_val);
    projection[12] = -(right + left) / (right - left);
    projection[13] = -(top + bottom) / (top - bottom);
    projection[14] = -(far_val + near_val) / (far_val - near_val);
    projection[15] = 1.0f;
}

// Helper to set up model matrix for a quad
static void setup_model_matrix(float *model, float x, float y, float width, float height) {
    memset(model, 0, 16 * sizeof(float));
    model[0] = width;
    model[5] = height;
    model[10] = 1.0f;
    model[12] = x;
    model[13] = y;
    model[15] = 1.0f;
}

void gl_renderer_draw_frame(GLRenderer *r, const WallpaperList *list, const Config *config) {
    // CRITICAL: Clear with transparent background!
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glUseProgram(r->shader_program);
    glBindVertexArray(r->vao);
    
    // CRITICAL: Update time uniform for animations!
    float current_time = (float)(SDL_GetTicks() - start_time) / 1000.0f;
    glUniform1f(glGetUniformLocation(r->shader_program, "time"), current_time);
    
    // =====================
    // SMOOTH SCROLL ANIMATION
    // =====================
    // Update target based on selected index
    r->target_scroll = (float)r->selected_index;
    
    // Smooth interpolation (ease-out style)
    // Adjust the 0.15f value to change animation speed (lower = slower)
    float scroll_speed = 0.12f;
    float diff = r->target_scroll - r->current_scroll;
    
    // Use smooth easing
    if (fabsf(diff) > 0.001f) {
        r->current_scroll += diff * scroll_speed;
    } else {
        r->current_scroll = r->target_scroll;
    }
    
    float windowSize[2] = {(float)config->window_width, (float)config->window_height};
    glUniform2fv(glGetUniformLocation(r->shader_program, "windowSize"), 1, windowSize);
    
    // Corner radius for rounded edges
    glUniform1f(glGetUniformLocation(r->shader_program, "cornerRadius"), 15.0f);
    
    int visible_count = wallpaper_list_visible_count(list);
    float projection[16], model[16];
    
    setup_ortho_projection(projection, (float)config->window_width, (float)config->window_height);
    glUniformMatrix4fv(glGetUniformLocation(r->shader_program, "projection"), 1, GL_FALSE, projection);
    
    // === FIRST PASS: Render frosted glass background ===
    Wallpaper *selected_wp = wallpaper_list_get((WallpaperList*)list, r->selected_index);
    if (selected_wp && selected_wp->thumb) {
        GLuint bg_texture;
        glGenTextures(1, &bg_texture);
        glBindTexture(GL_TEXTURE_2D, bg_texture);
        
        SDL_Surface *surf = selected_wp->thumb;
        const SDL_PixelFormatDetails *format_details = SDL_GetPixelFormatDetails(surf->format);
        GLenum format = (format_details->bytes_per_pixel == 4) ? GL_RGBA : GL_RGB;
        
        SDL_LockSurface(surf);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surf->w, surf->h, 0, format, GL_UNSIGNED_BYTE, surf->pixels);
        SDL_UnlockSurface(surf);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        // Set background mode
        glUniform1f(glGetUniformLocation(r->shader_program, "isBackground"), 1.0f);
        glUniform1f(glGetUniformLocation(r->shader_program, "selected"), 0.0f);
        glUniform1f(glGetUniformLocation(r->shader_program, "is3D"), 0.0f);
        glUniform1f(glGetUniformLocation(r->shader_program, "rotationY"), 0.0f);
        
        // Cover scaling (maintain aspect ratio)
        float window_aspect = (float)config->window_width / (float)config->window_height;
        float texture_aspect = (float)surf->w / (float)surf->h;
        
        float scale_w, scale_h;
        if (window_aspect > texture_aspect) {
            scale_w = config->window_width;
            scale_h = config->window_width / texture_aspect;
        } else {
            scale_h = config->window_height;
            scale_w = config->window_height * texture_aspect;
        }
        
        float offset_x = (config->window_width - scale_w) / 2.0f;
        float offset_y = (config->window_height - scale_h) / 2.0f;
        
        setup_model_matrix(model, offset_x, offset_y, scale_w, scale_h);
        glUniformMatrix4fv(glGetUniformLocation(r->shader_program, "model"), 1, GL_FALSE, model);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDeleteTextures(1, &bg_texture);
    }
    
    // === SECOND PASS: Render thumbnails with glow ===
    if (r->view_mode == 0) { // Horizontal carousel
        int center_x = config->window_width / 2;
        int center_y = config->window_height / 2;
        const int spacing = config->thumbnail_width + 30;
        
        // GLOW EXPANSION: Extra pixels around thumbnail for glow effect
        const float GLOW_PADDING = 50.0f;
        
        for (int i = 0; i < visible_count; i++) {
            Wallpaper *wp = wallpaper_list_get((WallpaperList*)list, i);
            if (wp && wp->thumb) {
                // USE SMOOTH SCROLL POSITION instead of integer selected_index
                float index_offset = (float)i - r->current_scroll;
                
                // Smooth horizontal position
                float x_offset = index_offset * spacing;
                
                // Scale based on distance from center (smooth!)
                float distance = fabsf(index_offset);
                float perspective_scale = 1.0f / (1.0f + distance * 0.15f);
                
                // Smooth "selected" value for glow intensity
                float selected = 1.0f - fminf(distance, 1.0f);
                selected = selected * selected; // Ease-out curve
                
                int scaled_width = (int)(config->thumbnail_width * perspective_scale);
                int scaled_height = (int)(config->thumbnail_height * perspective_scale);
                
                // Thumbnail position
                float thumb_x = center_x + x_offset - scaled_width / 2.0f;
                float thumb_y = center_y - scaled_height / 2.0f;
                
                // EXPANDED quad position (includes glow area)
                float quad_x = thumb_x - GLOW_PADDING;
                float quad_y = thumb_y - GLOW_PADDING;
                float quad_w = scaled_width + GLOW_PADDING * 2;
                float quad_h = scaled_height + GLOW_PADDING * 2;
                
                float rotation_y = index_offset * 0.1f;
                
                // Create texture
                GLuint texture;
                glGenTextures(1, &texture);
                glBindTexture(GL_TEXTURE_2D, texture);
                
                SDL_Surface *surf = wp->thumb;
                const SDL_PixelFormatDetails *format_details = SDL_GetPixelFormatDetails(surf->format);
                GLenum format = (format_details->bytes_per_pixel == 4) ? GL_RGBA : GL_RGB;
                
                SDL_LockSurface(surf);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surf->w, surf->h, 0, format, GL_UNSIGNED_BYTE, surf->pixels);
                SDL_UnlockSurface(surf);
                
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                
                // Set uniforms - use smooth selected value!
                glUniform1f(glGetUniformLocation(r->shader_program, "selected"), selected);
                glUniform1f(glGetUniformLocation(r->shader_program, "isBackground"), 0.0f);
                glUniform1f(glGetUniformLocation(r->shader_program, "is3D"), 0.0f);
                glUniform1f(glGetUniformLocation(r->shader_program, "rotationY"), rotation_y);
                glUniform1f(glGetUniformLocation(r->shader_program, "tiltX"), 0.0f);
                glUniform1f(glGetUniformLocation(r->shader_program, "depth3D"), 0.0f);
                
                // CRITICAL: Pass the ACTUAL thumbnail position and size
                float thumbnailPos[2] = {thumb_x, thumb_y};
                float thumbnailSize[2] = {(float)scaled_width, (float)scaled_height};
                glUniform2fv(glGetUniformLocation(r->shader_program, "thumbnailPos"), 1, thumbnailPos);
                glUniform2fv(glGetUniformLocation(r->shader_program, "thumbnailSize"), 1, thumbnailSize);
                
                // Average color for glow
                float avg_r, avg_g, avg_b;
                calculate_avg_color(surf, &avg_r, &avg_g, &avg_b);
                float avgColor[3] = {avg_r, avg_g, avg_b};
                glUniform3fv(glGetUniformLocation(r->shader_program, "avgColor"), 1, avgColor);
                
                // Model matrix for the EXPANDED quad (includes glow padding)
                setup_model_matrix(model, quad_x, quad_y, quad_w, quad_h);
                glUniformMatrix4fv(glGetUniformLocation(r->shader_program, "model"), 1, GL_FALSE, model);
                
                glDrawArrays(GL_TRIANGLES, 0, 6);
                glDeleteTextures(1, &texture);
            }
        }
    }
    
    glBindVertexArray(0);
    SDL_GL_SwapWindow(r->window);
}

void gl_renderer_cleanup(GLRenderer *r) {
    if (!r) return;
    
    if (r->vao) glDeleteVertexArrays(1, &r->vao);
    if (r->vbo) glDeleteBuffers(1, &r->vbo);
    if (r->shader_program) glDeleteProgram(r->shader_program);
    if (r->gl_context) SDL_GL_DestroyContext(r->gl_context);
    if (r->window) SDL_DestroyWindow(r->window);
    free(r);
}

#endif /* USE_SHADERS */