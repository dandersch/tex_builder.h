#include <GL/glew.h> // for loading gl functions, see https://www.glfw.org/docs/3.3/context_guide.html for manual loading
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <assert.h>
#include <string.h> // for memset

typedef unsigned int uint;
typedef struct vec3 { union { struct { float x,y,z; }; float e[3]; }; } vec3;
typedef struct vec4 { union { struct { float x,y,z,w; }; float e[4]; }; } vec4; // TODO use for vertex
typedef struct vertex_t {
    float x,y,z,w;
    float u,v; // NOTE unused
} vertex_t;

#define T(name, def) typedef struct name def name;
#define SHADER_CODE(...)
#include "common.h"
#undef T
#undef SHADER_CODE

#define _STRINGIFY(...) #__VA_ARGS__ "\n"
#define S(...) _STRINGIFY(__VA_ARGS__)
#define T(name,def) "struct " #name " " #def ";\n"
#define SHADER_CODE(...) S(__VA_ARGS__)
#define SHADER_VERSION_STRING "#version 430 core\n"

color_t tex_buf[TEXTURE_SIZE];

#ifdef COMPILE_DLL
#if defined(_MSC_VER)
    #define EXPORT __declspec(dllexport)
#else
    #define EXPORT __attribute__((visibility("default")))
#endif

typedef struct state_t
{
    int initialized;

    /* create texture */
    unsigned int texture_id;
    unsigned int texture_format;

    /* generate vao & vbo for texture */
    unsigned int texture_vbo;
    unsigned int texture_vao;

    /* create vertex shader */
    unsigned int vertex_shader_id;

    /* create fragment shader */
    unsigned int frag_shader_id;

    /* create shader */
    unsigned int shader_program_id;

    /* create compute shader & program */
    unsigned int compute_shader_id;
    unsigned int cs_program_id;

    double time;
} state_t;

void GLAPIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) { fprintf(stderr, "%s\n", message); }
EXPORT int on_load(state_t* state)
{
    /* init glew */
    {
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) { printf("Failed to initialize glew.\n"); }

        const GLubyte* renderer = glGetString( GL_RENDERER );
        const GLubyte* version  = glGetString( GL_VERSION );
        printf("Renderer: %s\n", renderer);
        printf("OpenGL version %s\n", version);
    }

    /* print out information about work group sizes */
    if (0)
    {
        int work_grp_cnt[3];
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);
        printf("Max work groups per compute shader\n");
        printf("   x: %i\n", work_grp_cnt[0]);
        printf("   y: %i\n", work_grp_cnt[1]);
        printf("   z: %i\n", work_grp_cnt[2]);
        printf("\n");

        int work_grp_size[3];
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
        printf("Max work group sizes\n");
        printf("   x: %i\n", work_grp_size[0]);
        printf("   y: %i\n", work_grp_size[1]);
        printf("   z: %i\n", work_grp_size[2]);

        int work_grp_inv;
        glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
        printf("Max invocations count per work group: %i\n", work_grp_inv);
        //exit(0);
    }

    /* enable debugging abilities */
    {
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(gl_debug_callback, NULL);
    }

    /* create texture */
    unsigned int* texture_id = &state->texture_id;
    unsigned int* texture_format = &state->texture_format;
    *texture_format = GL_RGBA8;
    {
        assert(glGetError() == GL_NO_ERROR);

        //glEnable(GL_TEXTURE_2D); // NOTE: causes error...
        glGenTextures(1, texture_id);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, *texture_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, *texture_format, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        //glBindTexture(GL_TEXTURE_2D, 0);
    }

    /* generate vao & vbo for texture */
    unsigned int* texture_vbo = &state->texture_vbo;
    unsigned int* texture_vao = &state->texture_vao;
    vertex_t vertices[] = {{-1, -1, 0, 1,    0, 0}, { 1, -1, 0, 1,    1, 0},
                           {-1,  1, 0, 1,    0, 1}, { 1,  1, 0, 1,    1, 1}};
    {
        assert(glGetError() == GL_NO_ERROR);

        glGenVertexArrays(1, texture_vao);
        glBindVertexArray(*texture_vao);

        glGenBuffers(1, texture_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, *texture_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_t), 0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*) offsetof(vertex_t, u));
        glEnableVertexAttribArray(1);
    }

    /* create vertex shader */
    unsigned int* vertex_shader_id = &state->vertex_shader_id;
    {
        assert(glGetError() == GL_NO_ERROR);

        *vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
        const char* vs_source = SHADER_VERSION_STRING S(
                                  layout(location=0) in vec4 pos;
                                  layout(location=1) in vec2 tex_pos;
                                  out vec2 o_tex_coord;
                                  void main(void) {
                                  gl_Position = pos;
                                  o_tex_coord = tex_pos;
                                });
        glShaderSource(*vertex_shader_id, 1, &vs_source, NULL);
        glCompileShader(*vertex_shader_id);

        /* print compile errors */
        int success;
        char infoLog[512];
        glGetShaderiv(*vertex_shader_id, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            glGetShaderInfoLog(*vertex_shader_id, 512, NULL, infoLog);
            printf("Vertex shader compilation failed: %s\n", infoLog);
        };
    }

    /* create fragment shader */
    unsigned int* frag_shader_id = &state->frag_shader_id;
    {
        assert(glGetError() == GL_NO_ERROR);

        *frag_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
        const char* fs_source = SHADER_VERSION_STRING S(
                                  in vec2 o_tex_coord;
                                  out vec4 color;
                                  uniform sampler2D u_texture;
                                  void main(void) {
                                  color = texture(u_texture, o_tex_coord);
                                });
        glShaderSource(*frag_shader_id, 1, &fs_source, NULL);
        glCompileShader(*frag_shader_id);

        /* print any compile errors */
        int success;
        char infoLog[512];
        glGetShaderiv(*frag_shader_id, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            glGetShaderInfoLog(*frag_shader_id, 512, NULL, infoLog);
            printf("Fragment shader compilation failed: %s\n", infoLog);
        };
    }

    /* create shader */
    unsigned int* shader_program_id = &state->shader_program_id;
    {
        assert(glGetError() == GL_NO_ERROR);

        *shader_program_id = glCreateProgram();
        glAttachShader(*shader_program_id, *vertex_shader_id);
        glAttachShader(*shader_program_id, *frag_shader_id);
        glLinkProgram(*shader_program_id);

        /* print any compile errors */
        int success;
        char infoLog[512];
        glGetProgramiv(*shader_program_id, GL_LINK_STATUS, &success);
        if(!success)
        {
            glGetProgramInfoLog(*shader_program_id, 512, NULL, infoLog);
            printf("Shader linking failed: %s\n", infoLog);
        }
    }

    /* create compute shader & program */
    unsigned int* compute_shader_id = &state->compute_shader_id;
    unsigned int* cs_program_id     = &state->cs_program_id;
    {
        assert(glGetError() == GL_NO_ERROR);

        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_2D, state->texture_id);
        glBindImageTexture(0, state->texture_id, 0, GL_FALSE, 0, GL_WRITE_ONLY, state->texture_format);


        assert(glGetError() == GL_NO_ERROR);

        *compute_shader_id = glCreateShader(GL_COMPUTE_SHADER);
        const char* cs_source =
                                #include "compute.glsl"
                                ;
        glShaderSource(*compute_shader_id, 1, &cs_source, NULL);
        glCompileShader(*compute_shader_id);

        assert(glGetError() == GL_NO_ERROR);

        /* print any compile errors */
        int success;
        char infoLog[512];
        glGetShaderiv(*compute_shader_id, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            glGetShaderInfoLog(*compute_shader_id, 512, NULL, infoLog);
            printf("%s\n", cs_source);
            printf("Compute shader compilation failed: %s\n", infoLog);
            return 0;
        };

        *cs_program_id = glCreateProgram();
        glAttachShader(*cs_program_id, *compute_shader_id);
        glLinkProgram(*cs_program_id);
        glDeleteShader(*compute_shader_id);

        /* print any linking errors */
        glGetProgramiv(*cs_program_id, GL_LINK_STATUS, &success);
        if(!success)
        {
            glGetProgramInfoLog(*cs_program_id, 512, NULL, infoLog);
            printf("Shader linking failed: %s\n", infoLog);
            return 0;
        }

        glUseProgram(*cs_program_id);

        assert(glGetError() == GL_NO_ERROR);
    }

    /* upload buffers to compute shader */
    {
        for (int i = 0; i < TEXTURE_SIZE; i++)
        {
            tex_buf[i] = (color_t){1,1,1,1};
        }

        GLuint ssbo; // shader storage buffer object
        glGenBuffers(1, &ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, TEXTURE_SIZE * sizeof(color_t), tex_buf, GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
    }

    if (!state->initialized)
    {
        state->initialized = 1;
    }

    return 1;
}

#include <math.h> // for fmod, sqrt, atan2, cos, sin, M_PI, ...
EXPORT void update(state_t* state, char input, double delta_cursor_x, double delta_cursor_y, double time)
{
    state->time = time;

    switch(input)
    {
        default: {} break;
    }
}

EXPORT void draw(state_t* state)
{
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glUseProgram(state->cs_program_id);

    /* upload uniforms */
    {
        glUniform1f(glGetUniformLocation(state->cs_program_id, "time"), (float) state->time);
    }

    glDispatchCompute(TEXTURE_WIDTH/WORK_GROUP_SIZE_X, TEXTURE_HEIGHT/WORK_GROUP_SIZE_Y, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    /* draw the texture */
    {
        glUseProgram(state->shader_program_id);
        glBindBuffer(GL_ARRAY_BUFFER, state->texture_vbo);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
}
#endif /* COMPILE_DLL */


#ifdef COMPILE_EXE
#ifndef COMPILE_DLL
#include <dlfcn.h>
#include <sys/stat.h>
#define DLL_FILENAME "./code.dll"
static void*  dll_handle;
static time_t dll_last_mod;
typedef struct state_t state_t;
static int  (*on_load)(state_t*);
static void (*update)(state_t*, char, double, double, double);
static void (*draw)(state_t*);
#endif

#include <stdlib.h>
#include <stdio.h>
int main()
{
    /* init glfw */
    GLFWwindow* window;
    {
        if (!glfwInit()) { printf("Failed to initalize glfw.\n"); }

        glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
        glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 ); // compute shaders added in 4.3
        glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
        glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT,1);

        glfwWindowHint(GLFW_FOCUSED, GLFW_FALSE);
        glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_FALSE);

        /* NOTE: experimenting with transparent windows */
        glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
        glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);

        window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
        if (!window) { printf("Failed to create GLFW window.\n"); glfwTerminate(); }

        glfwSetWindowTitle(window, WINDOW_TITLE);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // VSYNC
    }

    #ifndef COMPILE_DLL
    /* load in dll */
    dll_handle   = dlopen(DLL_FILENAME, RTLD_NOW);
    on_load      = dlsym(dll_handle, "on_load");
    update       = dlsym(dll_handle, "update");
    draw         = dlsym(dll_handle, "draw");
    struct stat attr;
    stat(DLL_FILENAME, &attr);
    dll_last_mod = attr.st_mtime;
    #endif

    state_t* state = malloc(1024 * 1024);
    memset(state, 0, 1024 * 1024);
    on_load(state);

    /* for some reason this hint is ignored when creating the window */
    glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);

    while (!glfwWindowShouldClose(window))
    {
        #ifndef COMPILE_DLL
        /* check if dll has changed on disk */
        if ((stat(DLL_FILENAME, &attr) == 0) && (dll_last_mod != attr.st_mtime))
        {
            printf("Attempting code hot reload...\n");

            if (dll_handle) /* unload dll */
            {
                dlclose(dll_handle);
                dll_handle = NULL;
                on_load    = NULL;
                update     = NULL;
                draw       = NULL;
            }
            dll_handle = dlopen(DLL_FILENAME, RTLD_NOW);
            if (dll_handle == NULL) { printf("Opening DLL failed. Trying again...\n"); }
            while (dll_handle == NULL) /* NOTE keep trying to load dll */
            {
                dll_handle = dlopen(DLL_FILENAME, RTLD_NOW);
            }
            on_load      = dlsym(dll_handle, "on_load");
            update       = dlsym(dll_handle, "update");
            draw         = dlsym(dll_handle, "draw");

            on_load(state);
            dll_last_mod = attr.st_mtime;
        }
        #endif

        static double time;
        float dt = glfwGetTime() - time;
        const double fps_cap = 1.f / 60.f;

        if (dt > fps_cap) {
            time = glfwGetTime();

            /* NOTE: do not set window title in a loop with uncapped fps */
            char fps_string[30];
            sprintf(fps_string, "%f", 1/dt);
            glfwSetWindowTitle(window, fps_string);

            /* poll events */
            static double cursor_x = 0, cursor_y = 0;
            {
                char input = ' ';

                glfwPollEvents();

                /* key inputs */
                if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) { glfwSetWindowShouldClose(window, 1); }
                if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)      { input = 'w'; }
                if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)      { input = 'a'; }
                if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)      { input = 's'; }
                if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)      { input = 'd'; }
                if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)      { input = 'q'; }
                if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)      { input = 'e'; }

                /* cursor pos */
                double x,y;
                glfwGetCursorPos(window, &x, &y);
                double dx = x - cursor_x;
                double dy = y - cursor_y;
                cursor_x = x;
                cursor_y = y;

                update(state, input, dx, dy, time);
            }


            draw(state);
            glfwSwapBuffers(window);
        }
    }

    printf("Terminated\n");

    return 0;
};
#endif /* COMPILE_EXE */
