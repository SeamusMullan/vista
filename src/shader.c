#ifdef USE_SHADERS

#include "shader.h"
#include "wallpaper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>

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
    
    // Check compilation
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
    r->view_mode = 0; // VIEW_MODE_HORIZONTAL
    r->grid_scroll_y = 0;
    r->target_scroll_y = 0.0f;
    r->current_scroll_y = 0.0f;
    r->search_mode = false;
    r->show_help = false;
    
    // Set OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    
    // Enable transparency
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    
    // Create window with transparency support
    r->window = SDL_CreateWindow(
        "vista - wallpaper switcher (OpenGL)",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        config->window_width,
        config->window_height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS
    );
    
    // Set window opacity (this may need compositor support on Linux)
    SDL_SetWindowOpacity(r->window, 0.95f);
    
    if (!r->window) {
        fprintf(stderr, "Failed to create window: %s\n", SDL_GetError());
        free(r);
        return NULL;
    }
    
    r->gl_context = SDL_GL_CreateContext(r->window);
    if (!r->gl_context) {
        fprintf(stderr, "Failed to create GL context: %s\n", SDL_GetError());
        SDL_DestroyWindow(r->window);
        free(r);
        return NULL;
    }
    
    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        SDL_GL_DeleteContext(r->gl_context);
        SDL_DestroyWindow(r->window);
        free(r);
        return NULL;
    }
    
    // Load shaders
    GLuint vertex_shader = shader_load("shaders/vertex.glsl", GL_VERTEX_SHADER);
    GLuint fragment_shader = shader_load("shaders/fragment.glsl", GL_FRAGMENT_SHADER);
    
    if (!vertex_shader || !fragment_shader) {
        fprintf(stderr, "Failed to load shaders\n");
        SDL_GL_DeleteContext(r->gl_context);
        SDL_DestroyWindow(r->window);
        free(r);
        return NULL;
    }
    
    // Create shader program
    r->shader_program = glCreateProgram();
    glAttachShader(r->shader_program, vertex_shader);
    glAttachShader(r->shader_program, fragment_shader);
    glLinkProgram(r->shader_program);
    
    // Check linking
    GLint success;
    glGetProgramiv(r->shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(r->shader_program, 512, NULL, log);
        fprintf(stderr, "Shader linking failed: %s\n", log);
    }
    
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    // Setup VAO, VBO, EBO for rendering quads
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
    
    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
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
    int step = 4; // Sample every 4th pixel for performance
    
    SDL_LockSurface(surf);
    
    for (int y = 0; y < surf->h; y += step) {
        for (int x = 0; x < surf->w; x += step) {
            Uint32 pixel = ((Uint32*)surf->pixels)[y * (surf->pitch / 4) + x];
            Uint8 pr, pg, pb;
            SDL_GetRGB(pixel, surf->format, &pr, &pg, &pb);
            
            sum_r += pr;
            sum_g += pg;
            sum_b += pb;
            sample_count++;
        }
    }
    
    SDL_UnlockSurface(surf);
    
    if (sample_count > 0) {
        *r = (sum_r / (float)sample_count) / 255.0f;
        *g = (sum_g / (float)sample_count) / 255.0f;
        *b = (sum_b / (float)sample_count) / 255.0f;
    } else {
        *r = *g = *b = 0.5f;
    }
}

void gl_renderer_draw_frame(GLRenderer *r, const WallpaperList *list, const Config *config) {
    // Smooth scroll animation
    const float smoothness = 0.15f;
    r->current_scroll += (r->target_scroll - r->current_scroll) * smoothness;
    r->current_scroll_y += (r->target_scroll_y - r->current_scroll_y) * smoothness;
    
    // Clear with transparency
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glUseProgram(r->shader_program);
    glBindVertexArray(r->vao);
    
    // Set time uniform for animation
    float time = SDL_GetTicks() / 1000.0f;
    glUniform1f(glGetUniformLocation(r->shader_program, "time"), time);
    
    // Set window size uniform
    float windowSize[2] = {(float)config->window_width, (float)config->window_height};
    glUniform2fv(glGetUniformLocation(r->shader_program, "windowSize"), 1, windowSize);
    
    // Set corner radius and blur strength
    glUniform1f(glGetUniformLocation(r->shader_program, "cornerRadius"), 15.0f);
    glUniform1f(glGetUniformLocation(r->shader_program, "blurStrength"), 8.0f);
    
    int visible_count = wallpaper_list_visible_count(list);
    
    if (r->view_mode == 0) { // VIEW_MODE_HORIZONTAL
        int x = 20 + (int)r->current_scroll;
        int y = (config->window_height - config->thumbnail_height) / 2;
        int spacing = 20;
        
        for (int i = 0; i < visible_count; i++) {
            Wallpaper *wp = wallpaper_list_get((WallpaperList*)list, i);
            if (wp && wp->thumb) {
                // Create OpenGL texture from SDL surface
                GLuint texture;
                glGenTextures(1, &texture);
                glBindTexture(GL_TEXTURE_2D, texture);
                
                SDL_Surface *surf = wp->thumb;
                GLenum format = (surf->format->BytesPerPixel == 4) ? GL_RGBA : GL_RGB;
                glTexImage2D(GL_TEXTURE_2D, 0, format, surf->w, surf->h, 0, format, GL_UNSIGNED_BYTE, surf->pixels);
                
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                
                // Set selected uniform (this enables the shader glow effect!)
                float selected = (i == r->selected_index) ? 1.0f : 0.0f;
                glUniform1f(glGetUniformLocation(r->shader_program, "selected"), selected);
                
                // Set thumbnail-specific uniforms
                glUniform1f(glGetUniformLocation(r->shader_program, "isBackground"), 0.0f);
                
                float thumbnailPos[2] = {(float)x, (float)y};
                float thumbnailSize[2] = {(float)config->thumbnail_width, (float)config->thumbnail_height};
                glUniform2fv(glGetUniformLocation(r->shader_program, "thumbnailPos"), 1, thumbnailPos);
                glUniform2fv(glGetUniformLocation(r->shader_program, "thumbnailSize"), 1, thumbnailSize);
                
                // Calculate and set average color for glow effect
                float avg_r, avg_g, avg_b;
                calculate_avg_color(surf, &avg_r, &avg_g, &avg_b);
                float avgColor[3] = {avg_r, avg_g, avg_b};
                glUniform3fv(glGetUniformLocation(r->shader_program, "avgColor"), 1, avgColor);
                
                // Setup transformation matrices
                float model[16], projection[16];
                
                // Orthographic projection
                float left = 0.0f, right = (float)config->window_width;
                float bottom = (float)config->window_height, top = 0.0f;
                projection[0] = 2.0f / (right - left); projection[1] = 0; projection[2] = 0; projection[3] = 0;
                projection[4] = 0; projection[5] = 2.0f / (top - bottom); projection[6] = 0; projection[7] = 0;
                projection[8] = 0; projection[9] = 0; projection[10] = -1; projection[11] = 0;
                projection[12] = -(right + left) / (right - left); projection[13] = -(top + bottom) / (top - bottom); projection[14] = 0; projection[15] = 1;
                
                // Model matrix (position and scale)
                model[0] = config->thumbnail_width; model[1] = 0; model[2] = 0; model[3] = 0;
                model[4] = 0; model[5] = config->thumbnail_height; model[6] = 0; model[7] = 0;
                model[8] = 0; model[9] = 0; model[10] = 1; model[11] = 0;
                model[12] = x; model[13] = y; model[14] = 0; model[15] = 1;
                
                glUniformMatrix4fv(glGetUniformLocation(r->shader_program, "projection"), 1, GL_FALSE, projection);
                glUniformMatrix4fv(glGetUniformLocation(r->shader_program, "model"), 1, GL_FALSE, model);
                
                // Draw quad
                glDrawArrays(GL_TRIANGLES, 0, 6);
                
                glDeleteTextures(1, &texture);
            }
            
            x += config->thumbnail_width + spacing;
        }
    }
    // TODO: Add grid mode rendering if needed
    
    glBindVertexArray(0);
    SDL_GL_SwapWindow(r->window);
}

void gl_renderer_cleanup(GLRenderer *r) {
    if (r->vao) glDeleteVertexArrays(1, &r->vao);
    if (r->vbo) glDeleteBuffers(1, &r->vbo);
    if (r->shader_program) glDeleteProgram(r->shader_program);
    if (r->gl_context) SDL_GL_DeleteContext(r->gl_context);
    if (r->window) SDL_DestroyWindow(r->window);
    free(r);
}

#endif /* USE_SHADERS */
