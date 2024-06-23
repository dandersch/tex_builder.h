#define TEX_BUILDER_IMPLEMENTATION
#include "../tex_builder.h"

#include "common.h"

#include <stdio.h>

/* usage */
__attribute__((visibility("default"))) int generate_textures(state_t* state)
{
    color_t RED   = {1,0,0,1};
    color_t GREEN = {0,1,0,1};

    texture_t tex  = tex_build(texture(32,32,GREEN), noise(1), test(RED) );

    state->tex = tex;

    texture_t tex2 = tex_build(tex, grunge(2));

    return 1;
}
