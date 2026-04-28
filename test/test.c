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

        /* sunset over water */
        texer_rect(0,64,32,32) {
            /* sky bands (top -> horizon) */
            color((color_t){lerp(zero_to_one, 0.2, 0.8), 0.05, 0.95 , 1});  /* deep purple sky */
            texer_rectcut_top(16) {
                texer_rectcut_bottom(4 * zero_to_one) {
                    color((color_t){0.9, 0.4, 0.2, 1});            /* orange band */
                    texer_rectcut_bottom(2 * zero_to_one) {
                        color((color_t){1.0, 0.85, 0.3, 1});       /* yellow horizon */
                    }
                }
            }

            /* sun: a 6x6 square that rises/sets with the timer */
            int sun_y = (int) lerp(zero_to_one, 4, 14);
            texer_rect(13, sun_y, 6, 6) {
                color((color_t){1.0, 0.9, 0.4, 1});
                /* clip the corners to fake a circle */
                texer_rect(0,0,1,1) { color(NONE); }
                texer_rect(5,0,1,1) { color(NONE); }
                texer_rect(0,5,1,1) { color(NONE); }
                texer_rect(5,5,1,1) { color(NONE); }
            }

            /* water (bottom half) */
            texer_rectcut_bottom(16) {
                color((color_t){0.1, 0.2, 0.5, 1});
                noise(0.2);

                /* wavering reflection of the sun */
                int refl_x = 13 + (int) lerp(zero_to_one, -2, 2);
                texer_rect(refl_x, 2, 6, 1) { color((color_t){1.0, 0.85, 0.3, 0.7}); }
                texer_rect(refl_x+1, 6, 4, 1) { color((color_t){1.0, 0.7, 0.25, 0.5}); }
                texer_rect(refl_x, 10, 6, 1) { color((color_t){0.9, 0.5, 0.2, 0.4}); }
            }

            outline(BLACK, 1) {}
        }

        /* treasure chest */
        texer_rect(32,64,32,32) {
            color(NONE);

            /* chest body (lower portion) */
            texer_rect(2, 12, 28, 18) {
                color(BROWN);
                noise(0.15);

                /* horizontal wood plank seams */
                texer_rect(0, 5,  28, 1) { color((color_t){0.3, 0.15, 0.0, 1}); }
                texer_rect(0, 11, 28, 1) { color((color_t){0.3, 0.15, 0.0, 1}); }

                /* iron bands on the sides */
                texer_rectcut_left(2)  { color((color_t){0.3, 0.25, 0.2, 1}); }
                texer_rectcut_right(2) { color((color_t){0.3, 0.25, 0.2, 1}); }

                /* lock plate */
                texer_rect(11, 4, 6, 8) {
                    color(YELLOW);
                    noise(0.1);
                    /* keyhole */
                    texer_rect(2, 3, 2, 3) { color(BLACK); }
                    outline(((color_t){0.5, 0.35, 0.0, 1}), 1) {}
                }
            }

            /* chest lid (upper portion) */
            texer_rect(2, 4, 28, 9) {
                color((color_t){0.5, 0.25, 0.0, 1});
                noise(0.15);

                /* lid lip / shadow under lid */
                texer_rectcut_bottom(1) { color((color_t){0.25, 0.12, 0.0, 1}); }

                /* iron straps on lid */
                texer_rectcut_left(2)  { color((color_t){0.3, 0.25, 0.2, 1}); }
                texer_rectcut_right(2) { color((color_t){0.3, 0.25, 0.2, 1}); }

                /* center iron strap */
                texer_rect(13, 0, 2, 9) { color((color_t){0.3, 0.25, 0.2, 1}); }
            }

            /* gold sparkle that orbits the lock */
            int sparkle_x = 16 + (int) lerp(zero_to_one,  6, -6);
            int sparkle_y = 18 + (int) lerp(zero_to_one, -3,  3);
            texer_rect(sparkle_x,     sparkle_y,     1, 1) { color(WHITE); }
            texer_rect(sparkle_x - 1, sparkle_y,     1, 1) { color(YELLOW); }
            texer_rect(sparkle_x + 1, sparkle_y,     1, 1) { color(YELLOW); }
            texer_rect(sparkle_x,     sparkle_y - 1, 1, 1) { color(YELLOW); }
            texer_rect(sparkle_x,     sparkle_y + 1, 1, 1) { color(YELLOW); }

            outline(BLACK, 1) {}
        }

        /* heartbeat monitor */
        texer_rect(64,64,32,32) {
            color(BLACK);

            /* faint grid lines */
            for (int gx = 0; gx < 4; gx++) {
                texer_rect(gx*8, 0, 1, 32) { color((color_t){0,0.15,0,1}); }
            }
            for (int gy = 0; gy < 4; gy++) {
                texer_rect(0, gy*8, 32, 1) { color((color_t){0,0.15,0,1}); }
            }

            /* baseline */
            texer_rect(0, 16, 32, 1) { color((color_t){0, 0.6, 0.1, 1}); }

            /* QRS spike: sweeps left->right, then wraps (sawtooth) */
            float sweep_t = fmodf(timer, 2.0f) / 2.0f; /* 0..1 then resets */
            int sweep_x = (int) lerp(sweep_t, 0, 31);
            /* small dip before */
            texer_rect(sweep_x - 2, 17, 1, 2) { color(GREEN); }
            /* tall up-spike */
            texer_rect(sweep_x - 1, 6,  1, 11) { color(GREEN); }
            /* down-spike */
            texer_rect(sweep_x,     17, 1, 7) { color(GREEN); }
            /* small bump after */
            texer_rect(sweep_x + 1, 14, 1, 2) { color(GREEN); }
            /* return to baseline */
            texer_rect(sweep_x + 2, 16, 1, 1) { color(WHITE); }

            /* scanline tail (fading) behind the sweep */
            texer_rect(sweep_x + 3, 16, 3, 1) { color((color_t){0, 0.4, 0.05, 0.7}); }

            outline(((color_t){0, 0.3, 0.05, 1}), 1) {}
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
