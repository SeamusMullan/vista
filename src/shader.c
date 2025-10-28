#ifdef USE_SHADERS

#include "shader.h"
#include <stdio.h>
#include <stdlib.h>

GLRenderer* gl_renderer_init(const Config *config) {
    // TODO: Implement OpenGL renderer
    fprintf(stderr, "OpenGL renderer not yet implemented\n");
    return NULL;
}

void gl_renderer_draw_frame(GLRenderer *r, const WallpaperList *list, int selected_index) {
    // TODO: Implement GL rendering
}

GLuint shader_load(const char *path, GLenum type) {
    // TODO: Implement shader loading
    return 0;
}

void gl_renderer_cleanup(GLRenderer *r) {
    // TODO: Implement cleanup
}

#endif /* USE_SHADERS */
