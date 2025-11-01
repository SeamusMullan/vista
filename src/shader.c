#ifdef USE_SHADERS

#include "shader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    
    // Set OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    
    // Create window
    r->window = SDL_CreateWindow(
        "vista - wallpaper switcher (OpenGL)",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        config->window_width,
        config->window_height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
    );
    
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

void gl_renderer_draw_frame(GLRenderer *r, const WallpaperList *list, int selected_index) {
    glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Basic rendering - just clear for now
    // Full implementation would render thumbnails with textures
    
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
