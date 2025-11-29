/**
 * @file shader.h
 * @brief OpenGL shader support (optional)
 */

#ifndef SHADER_H
#define SHADER_H

#ifdef USE_SHADERS

#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <stdbool.h>
#include "config.h"
#include "thumbnails.h"
#include "wallpaper.h"

/**
 * @brief OpenGL renderer state
 */
typedef struct {
    SDL_Window *window;
    SDL_GLContext gl_context;
    GLuint shader_program;
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    
    // Selection and scroll state (matching regular Renderer)
    int selected_index;
    int scroll_offset;
    float target_scroll;
    float current_scroll;
    int view_mode;           // 0 = horizontal, 1 = grid
    int grid_scroll_y;
    float target_scroll_y;
    float current_scroll_y;
    bool search_mode;
    bool show_help;
} GLRenderer;

/**
 * @brief Initialize OpenGL renderer
 * @param config Configuration
 * @return Initialized GL renderer
 */
GLRenderer* gl_renderer_init(const Config *config);

/**
 * @brief Render frame with OpenGL
 * @param r GL renderer
 * @param list Wallpaper list
 * @param config Configuration
 */
void gl_renderer_draw_frame(GLRenderer *r, const WallpaperList *list, const Config *config);

/**
 * @brief Cleanup OpenGL renderer
 * @param r GL renderer
 */
void gl_renderer_cleanup(GLRenderer *r);

/**
 * @brief Load and compile shader
 * @param path Shader file path
 * @param type Shader type (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER)
 * @return Compiled shader ID
 */
GLuint shader_load(const char *path, GLenum type);

/**
 * @brief Clean up GL renderer
 * @param r Renderer to free
 */
void gl_renderer_cleanup(GLRenderer *r);

#endif /* USE_SHADERS */

#endif /* SHADER_H */
