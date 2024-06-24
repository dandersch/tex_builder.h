#define TEX_BUILDER_IMPLEMENTATION
#include "../tex_builder.h"

#include "common.h"

#include <stdio.h>

/* usage */
__attribute__((visibility("default"))) int generate_textures(state_t* state, float dt)
{
    static float timer = 0; timer += dt; // if (timer > 1.0f) { timer = 0; }

    float zero_to_one = (sinf(timer) + 1)/2;

    color_t RED    = {1,0,0,1};
    color_t GREEN  = {0,1,0,1};
    color_t BLUE   = {0,0,1,1};

    color_t VIOLET = {0.8,0,0.4,1};


    /* NOTE: for opengl, end with flip() operation to match origin at top-left */
    texture_t tex  = tex_build(texture(32,32,GREEN),
                               noise(zero_to_one + 0.8), circle(16,16,(int)(zero_to_one*8),RED),
                               rect(0,(int) (zero_to_one * 29),32,4, VIOLET), rect(0,32,32,4,RED),
                               flip()
                              );

    //texture_t tex2 = tex_build(tex_copy(tex), noise(100));

    state->tex[0] = tex;

    return 1;
}
