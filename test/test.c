#define TEX_BUILDER_IMPLEMENTATION
#include "../tex_builder.h"

#include "common.h"

#include <stdio.h>

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
        scope_tex_build(atlas, texture(96,96, NONE)) {
            scope_tex_rect(0,0,32,32) {
                color(RED);
                noise(0.3);
                scope_tex_rect(10,20,10,10) {
                    color(BLUE);
                    pixel();
                    flip();
                    mirror();
                }
                scope_tex_rect(28,28,4,4) {
                    color(BLUE);
                    pixel();
                    mirror();
                }
            }
            scope_tex_rect(32,0,32,32)  { color(BROWN);   }
            scope_tex_rect(64,0,32,32)  { color(GREEN);   }
            scope_tex_rect(32,32,32,32) { color(YELLOW);  }
            scope_tex_rect(0,32,32,32)  { color(MAGENTA); }
            scope_tex_rect(64,32,32,32) { color(GRAY);    }
            scope_tex_rect(0,64,32,32)  { color(BLUE);    }
            scope_tex_rect(32,64,32,32) { color(ORANGE);  }
            scope_tex_rect(64,64,32,32) { color(CYAN);    }
            flip(); /* flip to match opengl's origin at top-left */
        }

        state->tex[0] = atlas;
    }


    return 1;
}
