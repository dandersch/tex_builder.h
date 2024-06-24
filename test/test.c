#define TEX_BUILDER_IMPLEMENTATION
#include "../tex_builder.h"

#include "common.h"

#include <stdio.h>

/* usage */
__attribute__((visibility("default"))) int generate_textures(state_t* state)
{
    color_t RED    = {1,0,0,1};
    color_t GREEN  = {0,1,0,1};
    color_t BLUE   = {0,0,1,1};

    color_t VIOLET = {1,0,1,1};

    /* NOTE: for opengl, end with flip() operation to match origin at top-left */
    texture_t tex  = tex_build(texture(32,32,GREEN),
                               noise(1.2), circle(16,26,8,RED), rect(5,18,4,4, RED),
                               rect(0,14,32,4,RED),
                               flip()
                              );

    texture_t tex2 = tex_build(tex_copy(tex), noise(100));

    state->tex = tex;

    return 1;
}
