#define TEX_BUILDER_IMPLEMENTATION
#include "../tex_builder.h"

#include "common.h"

#include <stdio.h>

#define TEXTURE_ATLAS_WIDTH  96
#define TEXTURE_ATLAS_HEIGHT 96

static color_t NONE    = {0,0,0,0};
static color_t RED     = {1,0,0,1};
static color_t GREEN   = {0,1,0,1};
static color_t BLUE    = {0,0,1,1};
static color_t BLACK   = {0, 0, 0, 1};
static color_t WHITE   = {1, 1, 1, 1};
static color_t YELLOW  = {1, 1, 0, 1};
static color_t CYAN    = {0, 1, 1, 1};
static color_t MAGENTA = {1, 0, 1, 1};
static color_t GRAY    = {0.5, 0.5, 0.5, 1};
static color_t ORANGE  = {1, 0.5, 0, 1};
static color_t PURPLE  = {0.5, 0, 0.5, 1};
static color_t BROWN   = {0.6, 0.3, 0, 1};
static color_t VIOLET  = {0.8,0,0.4,1};

__attribute__((visibility("default"))) int alloc_texture(state_t* state) {
    state->tex_builder = texture(TEXTURE_ATLAS_WIDTH, TEXTURE_ATLAS_HEIGHT);
    return 1;
}

#define lerp(t, a, b) (a + t * (b - a))

typedef struct thread_t { pthread_t id; int nr; int count; texture_t* tex; tex_builder_t builder; } thread_t;
void* tex_build(void* args)
{
    thread_t* t = (thread_t*) args;
    _scope_tex_build_threaded(*t->tex, t->builder, t->nr, t->count)
    {
        color(GRAY);
        pixel();
        scope_tex_rect(8,8,4,4) {
            color(BLUE);
            pixel();
            scope_tex_rect(0,0,16,16) {
                color(RED);
                noise(0.8);
                pixel();
            }
        }
        scope_tex_rect(32,0,32,32) {
            color(GREEN);
            pixel();
            scope_tex_rect(0,0,16,16) {
                color(RED);
                noise(0.8);
                pixel();
            }
        }
        scope_tex_rect(64,0,32,32) {
            color(GREEN);
            pixel();
            scope_tex_rect(0,16,16,16) {
                color(RED);
                noise(0.8);
                pixel();
            }
        }
        scope_tex_rect(0,32,32,32) {
            color(GREEN);
            pixel();
            scope_tex_rect(0,16,16,16) {
                color(RED);
                pixel();
            }
        }
        scope_tex_rect(0,64,32,32) {
            color(YELLOW);
            pixel();
            scope_tex_rect(0,16,16,16) {
                color(RED);
                pixel();
            }
        }
        scope_tex_rect(64,64,32,32) {
            color(YELLOW);
            pixel();
            scope_tex_rect(0,16,16,16) {
                color(RED);
                pixel();
            }
        }
        scope_tex_rect(32,64,32,32) {
            color(GREEN);
            pixel();
            scope_tex_rect(0,16,16,16) {
                color(RED);
                pixel();
            }
        }
        scope_tex_rect(64,32,32,32) {
            color(YELLOW);
            pixel();
            scope_tex_rect(0,16,16,16) {
                color(RED);
                pixel();
            }
        }
        scope_tex_rect(32,32,32,32) {
            color(GREEN);
            pixel();
            scope_tex_rect(8,8,24,33) {
                color(RED);
                noise(0.8);
                pixel();
            }
        }
    }

    return NULL;
}

#include <pthread.h>
__attribute__((visibility("default"))) int generate_textures(state_t* state, float dt) {
    /* animation test */
    static float timer = 0; timer += dt;
    float zero_to_one = (sinf(timer) + 1)/2;
    color_t _COLOR  = {zero_to_one,0,0.4,1};

    /* reset srand() */
    //static unsigned int random = 0;
    //srand(time(0) + random); // reseeds every frame
    srand(time(0)); // reseeds every second
    //srand(); // no reseeding
    //random = rand();

    texture_t atlas = {0};

    #define NUM_THREADS 8
    thread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        threads[i].count = NUM_THREADS;
        threads[i].nr       = i;
        threads[i].tex      = &atlas;
        threads[i].builder  = state->tex_builder;
        pthread_create(&threads[i].id, NULL, tex_build, (void*) &threads[i]);
    }

    #if 0
    scope_tex_build(atlas, state->tex_builder)
    {
        color(GRAY);
        scope_tex_rect(32,32,31,31) {
            color(GREEN);
            pixel();
            scope_tex_rect(8,8,24,33) {
                color(RED);
                noise(0.8);
                pixel();
            }
        }
        scope_tex_rect(0,0,32,32) {
            color(GREEN);
            pixel();
            scope_tex_rect(0,0,16,16) {
                color(RED);
                noise(0.8);
                pixel();
            }
        }
        scope_tex_rect(32,0,31,31) {
            color(GREEN);
            pixel();
            scope_tex_rect(0,0,16,16) {
                color(RED);
                noise(0.8);
                pixel();
            }
        }
        scope_tex_rect(64,0,32,32) {
            color(GREEN);
            pixel();
            scope_tex_rect(0,16,16,16) {
                color(RED);
                noise(0.8);
                pixel();
            }
        }
        scope_tex_rect(0,32,32,32) {
            color(GREEN);
            pixel();
            scope_tex_rect(0,16,16,16) {
                color(RED);
                pixel();
            }
        }
        scope_tex_rect(0,64,32,32) {
            color(YELLOW);
            pixel();
            scope_tex_rect(0,16,16,16) {
                color(RED);
                pixel();
            }
        }
        scope_tex_rect(64,64,32,32) {
            color(YELLOW);
            pixel();
            scope_tex_rect(0,16,16,16) {
                color(RED);
                pixel();
            }
        }
        scope_tex_rect(32,64,32,32) {
            color(GREEN);
            pixel();
            scope_tex_rect(0,16,16,16) {
                color(RED);
                pixel();
            }
        }
        scope_tex_rect(64,32,32,32) {
            color(GREEN);
            pixel();
            scope_tex_rect(0,16,16,16) {
                color(RED);
                pixel();
            }
        }
    }
    #endif

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i].id, NULL);
    }

    state->tex[0] = atlas;

    return 1;
}
