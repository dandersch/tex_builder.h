#define TEX_BUILDER_IMPLEMENTATION
#include "../tex_builder.h"

#include "common.h"

#include <stdio.h>


#define TEXTURE_ATLAS_WIDTH  96
#define TEXTURE_ATLAS_HEIGHT 96

__attribute__((visibility("default"))) int alloc_texture(state_t* state)
{
    state->tex_builder = texture(TEXTURE_ATLAS_WIDTH, TEXTURE_ATLAS_HEIGHT);

    return 1;
}

/* usage */
__attribute__((visibility("default"))) int generate_textures(state_t* state, float dt)
{
    static float timer = 0; timer += dt; // if (timer > 1.0f) { timer = 0; }

    float zero_to_one = (sinf(timer) + 1)/2;

    color_t RED     = {1,0,0,1};
    color_t GREEN   = {0,1,0,1};
    color_t BLUE    = {0,0,1,1};
    color_t BLACK   = {0, 0, 0, 1};
    color_t WHITE   = {1, 1, 1, 1};
    color_t YELLOW  = {1, 1, 0, 1};
    color_t CYAN    = {0, 1, 1, 1};
    color_t MAGENTA = {1, 0, 1, 1};
    color_t GRAY    = {0.5, 0.5, 0.5, 1};
    color_t ORANGE  = {1, 0.5, 0, 1};
    color_t PURPLE  = {0.5, 0, 0.5, 1};
    color_t BROWN   = {0.6, 0.3, 0, 1};
    color_t VIOLET  = {0.8,0,0.4,1};
    color_t _COLOR  = {zero_to_one,0,0.4,1};

    /* texture atlas usage code */
    {
        color_t NONE    = {0,0,0,0};

        #define lerp(t, a, b) (a + t * (b - a))

        /* api usage */
        texture_t atlas;
        scope_tex_build(atlas, state->tex_builder) {
            color(NONE);
            scope_tex_rect(0,0,32,32) {
                color(RED);
                noise(0.3);
                scope_tex_rect(10,10,10,10) {
                    color(BLUE);
                    pixel();
                    flip();
                    mirror();
                }
                scope_rectcut_top(8) {
                    color(YELLOW);
                }
                scope_rectcut_left(8) {
                    color(YELLOW);
                }
                scope_rectcut_bottom(8) {
                    color(YELLOW);
                }
                scope_rectcut_right(8) {
                    color(GRAY);
                }
            }
            scope_tex_rect(32,0,32,32)  {
                color(BROWN);
                scope_rectcut_top(8) {
                    pixel();
                    color(YELLOW);
                }
                scope_rectcut_left(8) {
                    color(YELLOW);
                }
                scope_rectcut_bottom(8) {
                    color(GRAY);
                }
            }
            scope_tex_rect(64,0,32,32)  { color(GREEN);   }
            scope_tex_rect(32,32,32,32) {
                color(YELLOW);
                scope_rectcut_top(8) {
                    color(GRAY);
                }
                scope_rectcut_bottom(8) {
                    color(GRAY);
                }
                scope_rectcut_right(8) {
                    color(GRAY);
                }
            }
            scope_tex_rect(0,32,32,32)  {
                color(MAGENTA);
                scope_rectcut_left(8) {
                    color(YELLOW);
                }
            }
            scope_tex_rect(64,32,32,32) {
                color(GRAY);
                scope_rectcut_left(8) {
                    color(YELLOW);
                }
            }
            scope_tex_rect(0,64,32,32)  { color(BLUE);    }
            scope_tex_rect(32,64,32,32) { color(ORANGE);  }
            scope_tex_rect(64,64,32,32) { color(CYAN);    }
            flip(); /* flip to match opengl's origin at top-left */
        }

        state->tex[0] = atlas;
    }


    return 1;
}
