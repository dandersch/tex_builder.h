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
    color_t BLACK   = {0, 0, 0, 1};
    color_t WHITE   = {1, 1, 1, 1};
    color_t YELLOW  = {1, 1, 0, 1};
    color_t CYAN    = {0, 1, 1, 1};
    color_t MAGENTA = {1, 0, 1, 1};
    color_t GRAY    = {0.5, 0.5, 0.5, 1};
    color_t ORANGE  = {1, 0.5, 0, 1};
    color_t PURPLE  = {0.5, 0, 0.5, 1};
    color_t BROWN   = {0.6, 0.3, 0, 1};
    color_t VIOLET = {0.8,0,0.4,1};

    color_t ANIMATED_COLOR = {zero_to_one,0,0.4,1};

    /* NOTE: for opengl, end with flip() operation to match origin at top-left */
    //texture_t tex  = tex_build(texture(32,32,GREEN),
    //                           noise(zero_to_one + 0.8), circle(16,16,(int)(zero_to_one*8),RED),
    //                           rect(0,(int) (zero_to_one * 29),32,4, VIOLET), rect(0,32,32,4,RED),
    //                           flip()
    //                          );

    //texture_t tex2 = tex_build(tex_copy(tex), noise(100));
    //state->tex[0] = tex;

    /* texture atlas usage code */
    {
        color_t NONE    = {0,0,0,0};
        //texture_t atlas = tex_build(texture(96,96, NONE));
        //texture_t tex_1 = tex_build(_subtexture(&atlas,32,32,RED), noise(zero_to_one + 0.1));
        //texture_t tex_2 = tex_build(_subtexture(&atlas,32,32,BLUE));
        //texture_t tex_3 = tex_build(_subtexture(&atlas,32,32,GREEN));
        //texture_t tex_4 = tex_build(_subtexture(&atlas,32,32,VIOLET));
        //texture_t tex_X = tex_build(_subtexture(&atlas,32,32,GREEN),
        //                           noise(zero_to_one + 0.8), circle(16,16,(int)(zero_to_one*8),RED),
        //                           rect(0,(int) (zero_to_one * 29),32,4, VIOLET), rect(0,32,32,4,RED),
        //                           flip()
        //                          );
        //texture_t tex_5 = tex_build(_subtexture(&atlas,32,32,YELLOW));
        //texture_t tex_6 = tex_build(_subtexture(&atlas,32,32,BROWN));
        //texture_t tex_7 = tex_build(_subtexture(&atlas,32,32,ANIMATED_COLOR));
        //texture_t tex_8 = tex_build(_subtexture(&atlas,32,32,CYAN));

        //texture_t tex_9 = tex_build(_subtexture(&atlas,32,32,PURPLE));

        // scope_tex_build(atlas, texture(96,96))
        // {
        //     color(RED)   // _color(temp, ...);
        //     noise(1)     // _noise(temp, ...);
        //     scope_tex_rect() // temp.         temp.x_pos = x_pos;
        //         size_t x_start, y_start; size_t width, height;
        //     for (size_t old_x_start = temp.x_start, old_y_start = temp.y_start, ; temp.j == 0; (temp.i += 1, atlas = _create(atlas)))
        //     {
        //
        //     }
        // }

        #define lerp(t, a, b) (a + t * (b - a))

        /* new api */
        texture_t atlas;
        scope_tex_build(atlas, texture(96,96, GREEN)) {
            scope_tex_rect(44,8,64,16) {
                color(RED);
                scope_tex_rect(4,lerp(zero_to_one,4,44),16,8) {
                    color(BLUE);
                    noise(0.8);
                    pixel();
                }
            }
            flip();
        }

        /* stress test */
        //texture_t atlas = tex_build(texture(96,96, GREEN));
        //{
        //    for (int i = 0; i < 7; i++)
        //    {
        //        tex_build(_subtexture(&atlas,32,32,ANIMATED_COLOR),
        //                  noise(zero_to_one + 0.2)
        //                  //, circle(16,16,(int)(zero_to_one*8),RED)
        //        );
        //    }
        //}

        state->tex[0] = atlas;
    }


    return 1;
}
