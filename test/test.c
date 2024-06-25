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
__attribute__((visibility("default"))) int generate_textures(state_t* state, float dt)
{
    static float timer = 0; timer += dt;
    float zero_to_one = (sinf(timer) + 1)/2;
    color_t _COLOR  = {zero_to_one,0,0.4,1};

    /* texture building usage code */
    texture_t atlas;
    scope_tex_build(atlas, state->tex_builder) {
        color(NONE);

        /* creeper face */
        scope_tex_rect(0,0,32,32)   {
            color(GREEN);
            noise(1.0);
            scope_tex_rect(4,8,8,8) {
                color(BLACK);
                scope_tex_rect(2,2,4,4) {
                    color(_COLOR);
                }
            }
            scope_tex_rect(20,8,8,8) {
                color(BLACK);
                scope_tex_rect(2,2,4,4) {
                    color(_COLOR);
                }
            }
            scope_tex_rect(12,16,16,8) { color(BLACK); }
            scope_tex_rect(8,20,16,16) { color(BLACK); }
            scope_tex_rect(12,(int) lerp(zero_to_one, 28, 33),16,8)  { color(GREEN); noise(1.0); } // NOTE: not properly cut off
        }

        /* sliding door */
        scope_tex_rect(32,0,32,32)  {
            color(BLACK);
            scope_rectcut_left((int) lerp(zero_to_one, 18, 0)) {
                color(GRAY);
                noise(0.3);
                scope_rectcut_right(3) {
                  color((color_t){0.2,0.3,0.5,1});
                }
            }
            scope_rectcut_right((int) lerp(zero_to_one, 18, 0)) {
                color(GRAY);
                noise(0.3);
                scope_rectcut_left(3) {
                  color((color_t){0.2,0.5,0.5,1});
                }
            }
        }

        /* pong animation */
        scope_tex_rect(64,0,32,32)  {
            color(GRAY);
            noise(0.1);
            rect(lerp(zero_to_one,2,28), lerp(zero_to_one,3,28),3,3,WHITE);
            scope_rectcut_left(2)  { rect(0, lerp(zero_to_one,0,25),2,8,WHITE); }
            scope_rectcut_right(2) { rect(0, lerp(zero_to_one,0,25),2,8,WHITE); }
        }

        scope_tex_rect(0,32,32,32)  {
            color(BLUE);
        }

        /* testing clamping */
        scope_tex_rect(32,32,32,32) {
            color(BLACK);
            scope_tex_rect(0, (int) lerp(zero_to_one, 0, 33),32,32) {
                color(ORANGE);
                scope_rectcut_left(10)  { color(YELLOW); }
                scope_rectcut_right(10) { color(YELLOW); }
                scope_rectcut_top(5)    { color(ORANGE); }
            }
        }

        scope_tex_rect(64,32,32,32) {
            color(GRAY);
        }

        /* using for loops for generating */
        for (int i = 0; i < 3; i++) {
            scope_tex_rect(32 * i,64,32,32) {
                switch (i) {
                    case 0: { color(YELLOW);  } break;
                    //case 1: { color(MAGENTA); } break;
                    case 2: { color(CYAN);    } break;
                }
            }
        }
        flip(); /* flip to match opengl's origin at top-left */
    }

    state->tex[0] = atlas;

    return 1;
}
