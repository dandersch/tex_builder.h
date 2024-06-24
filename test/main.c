#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_loadso.h>  // cross platform dll loading

#include "../tex_builder.h"
#include "common.h" // contains shared state definition

#include <assert.h>

/* HOT RELOAD */
#include <sys/stat.h>
#define DLL_FILENAME "./code.dll"
static int    (*generate_textures)(state_t*, float);
static time_t dll_last_mod;
static void*  dll_handle;
static state_t* state = NULL;

static SDL_GLContext context;
static SDL_Window* window;

#define WINDOW_WIDTH  256
#define WINDOW_HEIGHT 256

#define SHADER_STRINGIFY(x) "#version 330\n" #x
const char* vertex_shader_source = SHADER_STRINGIFY(

    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 tex_coords;

    uniform mat4 orthoProjection;
    uniform mat4 view_matrix;
    uniform sampler2D tex;
    uniform float time;

    out vec2 o_tex_coords;

    void main()
    {
        gl_Position = orthoProjection * view_matrix * vec4(aPos,1.0);
        o_tex_coords = tex_coords;
    };
);
const char* fragment_shader_source = SHADER_STRINGIFY(

    out vec4 FragColor;

    in vec2 o_tex_coords;
    in float o_time;

    uniform sampler2D tex;

    void main()
    {
        FragColor   = texture(tex, o_tex_coords);
    }
);

static GLuint VAO;
static GLuint VBO;
static GLuint shader_id;
static const int tex_mode = GL_RGBA32F;

#define vertex_t(LAYOUT) \
    LAYOUT(     0,     3, GL_FLOAT, float, vert_x, float vert_y; float vert_z; ) \
    LAYOUT(     1,     2, GL_FLOAT, float, tex_x,  float tex_y;                )
          /*index, count, ogl_type,  type, member, va_args                    */
#define FILL_MEMBERS(layout, count, ogl_type, type, member, ...) type member; __VA_ARGS__
typedef struct vertex_t {
    vertex_t(FILL_MEMBERS)
} vertex_t;
const float x = 0; // position for rendered quad on screen
const float y = 0; // position for rendered quad on screen
const float tex_size_x = (float) 256; // NOTE rename to quad_size
const float tex_size_y = (float) 256; // NOTE rename to quad_size
// vertices for a quad
const vertex_t vertices[] = {
     // bottom right tri
     {
         x,        y,        0.0f, // bottom left
         0.0f,     0.0f,           // uv
     },
     {
         x + tex_size_x, y,        0.0f, // bottom right
         1.0f,           0.0f,           // uv
     },
     {
         x + tex_size_x, y + tex_size_y, 0.0f, // top right
         1.0f,          1.0f,                  // uv
     },

     // upper left tri
     {
        x + tex_size_x, y + tex_size_y, 0.0f, // top right
        1.0f,           1.0f,                 // uv
     },
     {
        x, y + tex_size_y, 0.0f,  // top left
        0.0f,  1.0f,              // uv
     },
     {
        x,        y,        0.0f, // bottom left
        0.0f,     0.0f,           // uv
     }
};

/* helper function */
GLuint compile_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        fprintf(stderr, "Shader Compilation failed: %s\n", infoLog);
        return 0;
    }
    return shader;
}

int upload_textures(state_t* state)
{
    //glTexImage2D(GL_TEXTURE_2D, 0, tex_mode, state->tex.width, state->tex.height, 0, tex_mode, GL_UNSIGNED_BYTE, state->tex.rgb);
    glTexImage2D(GL_TEXTURE_2D, 0, tex_mode, state->tex[0].width, state->tex[0].height, 0, GL_RGBA, GL_FLOAT, state->tex[0].rgb);
    //glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
    return 1;
}

int platform_load_code()
{
    if (dll_handle) /* unload old dll */
    {
        SDL_UnloadObject(dll_handle);
        generate_textures = NULL;
        dll_handle        = NULL;
    }

    dll_handle = SDL_LoadObject(DLL_FILENAME);
    if (dll_handle == NULL) { printf("Opening DLL failed. Trying again...\n"); }
    while (dll_handle == NULL) /* NOTE keep trying to load dll */
    {
        dll_handle = SDL_LoadObject(DLL_FILENAME);
    }

    generate_textures = (int (*)(state_t*,float)) SDL_LoadFunction(dll_handle, "generate_textures");
    if (!generate_textures) { printf("Error finding function\n"); return 0; }

    return 1;
}

void GLAPIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {fprintf(stderr, "%s\n", message);}
int main(int argc, char* args[])
{
    /* sdl initialization boilerplate */
    {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) { fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError()); return -1; }
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG); // only for 4.3+
        window = SDL_CreateWindow("OpenGL Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
        if (!window) { fprintf(stderr, "Failed to create SDL window: %s\n", SDL_GetError()); return -1; }
        context = SDL_GL_CreateContext(window); // opengl context
        if (!context) { fprintf(stderr, "Failed to create OpenGL context: %s\n", SDL_GetError()); return -1; }
    }

    // initial loading of dll
    int code_loaded = platform_load_code();
    if (!code_loaded) { fprintf(stderr, "Couldn't load dll\n"); exit(-1); }
    struct stat attr;
    stat(DLL_FILENAME, &attr);
    dll_last_mod = attr.st_mtime;

    state = malloc(sizeof(state_t));
    generate_textures(state,0);

    /* init glew, vao, vbo, texture & upload texture */
    {
        // Initialize GLEW
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) { fprintf(stderr, "Failed to initialize GLEW\n"); return -1; }

        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(gl_debug_callback, NULL);

        /* generate and bind vertex array object and vertex buffer object */
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);

        /* upload vertices */
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // attrib pointers
        #define FILL_ATTRIB_POINTER(index, count, ogl_type, type, member, ...)  \
          glVertexAttribPointer(index, count, ogl_type, GL_FALSE, sizeof(vertex_t), (void*) offsetof(vertex_t, member)); \
          glEnableVertexAttribArray(index);
        vertex_t(FILL_ATTRIB_POINTER)

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        /* generate texture */
        GLuint tex_id;
        glGenTextures(1, &tex_id);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex_id);

        // how to sample the texture when its larger or smaller
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        /* upload texture */
        upload_textures(state);

        /* enable blending for transparency */
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_SRC_ALPHA);
    }

    assert(glGetError() == GL_NO_ERROR);

    /* create shader */
    {
        GLuint vertex_shader   = compile_shader(GL_VERTEX_SHADER, vertex_shader_source);
        GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_source);

        if (!vertex_shader)   { return 0; }
        if (!fragment_shader) { return 0; }

        shader_id = glCreateProgram();
        glAttachShader(shader_id, vertex_shader);
        glAttachShader(shader_id, fragment_shader);
        glLinkProgram(shader_id);

        GLint success;
        GLchar infoLog[512];
        glGetProgramiv(shader_id, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader_id, 512, NULL, infoLog);
            fprintf(stderr, "Linking failed\n%s\n", infoLog);
            exit(EXIT_FAILURE);
        }

        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
    }

    /* upload uniforms */
    {
        /* orthographic projection */
        glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT); // NOTE hardcoded

        float left   = 0.0f;
        float right  = (float) WINDOW_WIDTH;
        float bottom = 0.0f;
        float top    = (float) WINDOW_HEIGHT;
        float near   = -1.0f;
        float far    = 1.0f;

        float orthoProjection[16] = {
            2.0f / (right - left),                                        0.0f,                         0.0f, 0.0f,
            0.0f,                                        2.0f / (top - bottom),                         0.0f, 0.0f,
            0.0f,                                                         0.0f,         -2.0f / (far - near), 0.0f,
            -(right + left) / (right - left), -(top + bottom) / (top - bottom), -(far + near) / (far - near), 1.0f
        };

        /* upload uniforms */
        glUseProgram(shader_id);
        int orthoLocation = glGetUniformLocation(shader_id, "orthoProjection");
        if (orthoLocation == -1) { printf("Uniform orthoProjection not found\n"); }
        glUniformMatrix4fv(orthoLocation, 1, GL_FALSE, orthoProjection);

        float cam_pos_x = 0, cam_pos_y = 0;
        float view_matrix[16] = {
            1.0f,  0.0f, 0.0f, 0.0f,
            0.0f,  1.0f, 0.0f, 0.0f,
            0.0f,  0.0f, 1.0f, 0.0f,
            -cam_pos_x,-cam_pos_y, 0.0f, 1.0f,
        };
        int view_matrix_uniform_location = glGetUniformLocation(shader_id, "view_matrix");
        if (view_matrix_uniform_location == -1) { printf("Uniform view_matrix not found\n"); }
        glUniformMatrix4fv(view_matrix_uniform_location, 1, GL_FALSE, view_matrix);
    }

    int running = 1;
    SDL_Event event;
    int mouse_x, mouse_y;
    float pos_x = 0, pos_y = 0;

    SDL_GL_SetSwapInterval(1); // turn vsync on/off

    unsigned int current_time = 0;
    unsigned int last_time    = SDL_GetTicks();
    float dt                  = 0.0f;
    while (running)
    {
        current_time = SDL_GetTicks();
        dt = (current_time - last_time) / 1000.0f; // milliseconds to seconds
        last_time = current_time;

        /* print out fps every second */
        {
            static float timer  = 0;
            timer += dt;
            if (timer > 1.0f) {
                printf("FPS: %f\n", 1.0f/dt);
                timer = 0;
            }
        }

        /* check if dll has changed on disk */
        if ((stat(DLL_FILENAME, &attr) == 0) && (dll_last_mod != attr.st_mtime))
        {
            printf("Attempting code hot reload...\n");
            platform_load_code();
            dll_last_mod = attr.st_mtime;

        }

        /* generate textures again */
        {
            generate_textures(state,dt);
            upload_textures(state);
            /* free allocated textures */
            for (int i = 0; i < TEXTURE_COUNT; i ++)
            {
                free(state->tex[i].rgb);
            }
        }

        /* event handling */
        while (SDL_PollEvent(&event)) {
            switch (event.type)
            {
                case SDL_QUIT: { running = 0; } break;
            }
        }

        /*  render */
        {
            glClear(GL_COLOR_BUFFER_BIT);
            glClearColor(0.1f, 0.2f, 0.1f, 0.2f);

            glUseProgram(shader_id);

            /* TODO upload uniforms */

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
        }
        SDL_GL_SwapWindow(window);
    }

    return 0;
}
