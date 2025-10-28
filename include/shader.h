/**
 * @file shader.h
 * @brief OpenGL shader support (optional)
 */

#ifndef SHADER_H
#define SHADER_H

#ifdef USE_SHADERS

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include "config.h"
#include "thumbnails.h"

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
 * @param selected_index Currently selected index
 */
void gl_renderer_draw_frame(GLRenderer *r, const WallpaperList *list, int selected_index);

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
