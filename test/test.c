#define TEXER_IMPLEMENTATION
#include "../texer.h"

#include "common.h"

#include <math.h>   // for sinf
#include <time.h>   // for seeding srand()
#include <stdlib.h> // for rand()

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
    state->texer = texture(TEXTURE_ATLAS_WIDTH, TEXTURE_ATLAS_HEIGHT);
    return 1;
}

#define lerp(t, a, b) (a + t * (b - a))
static float timer = 0;
static uint random_seed_per_sec   = 0;
static uint random_seed_per_frame = 0;

typedef struct thread_t { pthread_t id; int nr; int count; texture_t* tex; texer_t builder; } thread_t;
void* tex_build(void* args)
{
    float zero_to_one = (sinf(timer) + 1)/2;
    color_t _COLOR  = {zero_to_one,0,0.4,1};

    thread_t* t = (thread_t*) args;
    texer_threaded(*t->tex, t->builder, t->nr, t->count) {
        color(NONE);

        /* creeper face */
        texer_rect(0,0,32,32)   {
            color(GREEN);
            seed((int)rand());
            //seed(random_seed_per_frame);
            noise(1.0);
            texer_rect(4,8,8,8) {
                color(BLACK);
                texer_rect(2,2,4,4) {
                    color(_COLOR);
                }
            }
            texer_rect(20,8,8,8) {
                color(BLACK);
                texer_rect(2,2,4,4) {
                    color(_COLOR);
                }
            }
            texer_rect(12,16,8,16) { color(BLACK); }
            texer_rect(8,20,16,16) { color(BLACK); }
            texer_rect(12,(int) lerp(zero_to_one, 28, 33), 8,16)  { color(GREEN); noise(1.0); } // NOTE: not properly cut off
        }

        /* sliding door */
        texer_rect(32,0,32,32)  {
            color(BLACK);
            texer_rectcut_left((int) lerp(zero_to_one, 18, 0))
            {
                color(GRAY);
                noise(0.3);
                texer_rectcut_right(3) {
                  color((color_t){0.2,0.3,0.5,1});
                }
            }
            texer_rectcut_right((int) lerp(zero_to_one, 18, 0)) {
                color(GRAY);
                noise(0.3);
                texer_rectcut_left(3) {
                  color((color_t){0.2,0.5,0.5,1});
                }
            }
        }

        /* pong animation */
        texer_rect(64,0,32,32)  {
            color(GRAY);
            noise(0.1);

            texer_rect(lerp(zero_to_one,2,28),lerp(zero_to_one,3,28),3,3) { color(WHITE); }
            texer_rectcut_left(2)  {
                texer_rect(0, lerp(zero_to_one,0,25),2,8) color(WHITE);
            }
            texer_rectcut_right(2) {
                texer_rect(0, lerp(zero_to_one,0,25),2,8) color(WHITE);
            }
        }

        /* zoom-in */
        texer_rect(0,32,32,32)  {
            color(BLUE);
            unsigned int t_ = lerp(zero_to_one,1,20);
            outline(RED, t_) {
                outline(GRAY,t_) {
                    outline(YELLOW,t_) {
                        outline(CYAN,t_) {
                            outline(GREEN,t_) {
                            }
                        }
                    }
                }
            }
        }

        /* testing clamping */
        texer_rect(32,32,32,32) {
            color(BLACK);
            texer_rect(0, (int) lerp(zero_to_one, 0, 33),32,32) {
                color(ORANGE);
                texer_rectcut_left(10)  { color(YELLOW); }
                texer_rectcut_right(10) { color(YELLOW); }
                texer_rectcut_top(5)    { color(ORANGE); }
            }
        }

        /* art painting (TODO turn into shattered mirror) */
        texer_rect(64,32,32,32) {
            color(GRAY);
            outline(BROWN, 2) {
                seed(1);
                voronoi(10);
            }
        }

        texer_rect(0,64,32,32) {
            color(BLACK);
        }

        /* using for loops for generating */
        for (int i = 0; i < 3; i++) {
            texer_rect(32 * i,64,32,32) {
                switch (i) {
                    case 1: { color(MAGENTA); } break;
                    case 2: { color(CYAN);    } break;
                }
            }
        }
    }

    return NULL;
}

#include <pthread.h>
__attribute__((visibility("default"))) int generate_textures(state_t* state, float dt) {
    /* animation test */
    timer += dt;

    /* reset srand() */

    /* reseeds every frame */
    srand(time(0) + random_seed_per_frame);
    random_seed_per_frame = (uint) rand();

    /* reseed every second */
    //srand(time(0));
    //random_seed_per_sec   = (uint) rand();

    texture_t atlas = {0};

    #define NUM_THREADS 8
    thread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        threads[i].count = NUM_THREADS;
        threads[i].nr       = i;
        threads[i].tex      = &atlas;
        threads[i].builder  = state->texer;
        pthread_create(&threads[i].id, NULL, tex_build, (void*) &threads[i]);
    }

    #if 0
    scope_tex_build(atlas, state->texer)
    {
        color(GRAY);
        texer_rect(32,32,31,31) {
            color(GREEN);
            pixel();
            texer_rect(8,8,24,33) {
                color(RED);
                noise(0.8);
                pixel();
            }
        }
        texer_rect(0,0,32,32) {
            color(GREEN);
            pixel();
            texer_rect(0,0,16,16) {
                color(RED);
                noise(0.8);
                pixel();
            }
        }
        texer_rect(32,0,31,31) {
            color(GREEN);
            pixel();
            texer_rect(0,0,16,16) {
                color(RED);
                noise(0.8);
                pixel();
            }
        }
        texer_rect(64,0,32,32) {
            color(GREEN);
            pixel();
            texer_rect(0,16,16,16) {
                color(RED);
                noise(0.8);
                pixel();
            }
        }
        texer_rect(0,32,32,32) {
            color(GREEN);
            pixel();
            texer_rect(0,16,16,16) {
                color(RED);
                pixel();
            }
        }
        texer_rect(0,64,32,32) {
            color(YELLOW);
            pixel();
            texer_rect(0,16,16,16) {
                color(RED);
                pixel();
            }
        }
        texer_rect(64,64,32,32) {
            color(YELLOW);
            pixel();
            texer_rect(0,16,16,16) {
                color(RED);
                pixel();
            }
        }
        texer_rect(32,64,32,32) {
            color(GREEN);
            pixel();
            texer_rect(0,16,16,16) {
                color(RED);
                pixel();
            }
        }
        texer_rect(64,32,32,32) {
            color(GREEN);
            pixel();
            texer_rect(0,16,16,16) {
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
