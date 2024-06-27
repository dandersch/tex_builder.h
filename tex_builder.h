#pragma once
/*
   IDEA: an immediate-mode texture generation api in C that is nice to use

   inspiration:
   https://x.com/phoboslab/status/1432363394418491394/photo/1

   Limitations:
   - scope_... macros cannot be on the same line
*/

/* Possible extensions:
 *
 * combining textures:
 *   stencil/blit(t,x,y): draws tex t to the texture at (x,y)
 *   blend:  blend two textures together with a specified blending mode (e.g., alpha blending, additive blending)
 *
 * drawing operations:
 *   line:    draw a line segment between two points with a color
 *   voronoi: draw a voronoi pattern with an outline color
 *   rotate:  rotate the texture by a specified angle around a pivot point.
 *   text:    draw string with font at position with size or fit the rect
 *   resize:  resize the texture to a new width and height using interpolation (e.g., bilinear interpolation).
 *
 * scopes:
 *   scope_centered(): draw everything at the center
 *   scope_tex_push_rect(w,h): push a texture of size h,w onto the current texture without
 *                             caring where it ends up (useful for building atlases).
 *                             Maybe make it so that a rect_t can be passed in that gets filled with x,y,h,w.
 *
 * general:
 *   outline version of rect, circle.
 *   NOTE: outline could be implemented in terms of a scope_rect if we had masks
 *   scope_rect(0+thickness, 0+thickness, w-thickness, h-thickness) { color(COLOR); }
 *
 *   all drawing operations should have versions that take float (i.e. percentages) instead of pixels
 *   all drawing operations should take in alpha values into account
 *
 *   if we have consider alpha values, we can have masks inside tex_builder_t to implement e.g. scope_tex_circle() {}
 *
 *   push/pop pragma warning about shadowing variables when nesting scope_tex_build's
 *
 *   make work in glsl shader code
 *   check if it works with C++ and MSVC
 *   generate normal maps along texture for operations that add depth (e.g. insets)
 *
 *   flags(MIRROR|BLEND_ALPHA);
 */


/* GLSL restrictions:
 * - no pointers
 * - no function forward declarations
 * - no size_t
 * - no 'unsigned int', only uint
 * - no '(color_t){1,1,1,1}' initialization, only 'color_t(1,1,1,1)'
*/

#include <stddef.h>

#ifndef RUN_ON_COMPUTE_SHADER
/* TODO use these */
typedef unsigned int uint;
typedef struct color_t color_t;
typedef struct texture_t texture_t;
#endif

// NOTE RGBA vs BGRA layout could be set with a macro
struct color_t       { float r; float g; float b; float a;        };
struct texture_t     { size_t width; size_t height; color_t* rgb; };

/* used internally */
typedef struct tex_builder_t {
    texture_t tex;

    /* size of the whole texture NOTE: same as tex.{width,height} */
    size_t atlas_width;
    size_t atlas_height;

    // int flags; // TODO bitfield containing e.g. TEX_BUILD_{FLIP,MIRROR,BLEND_ALPHA}

    struct {
        int x, y;
        int w, h;
    } mask;

    /* used in for-loop macros */
    int i;
} tex_builder_t;

/* NOTE: turn into flags that can be set by user */
#define TEX_BUILDER_FLAG_FLIP 1

/*
 * api
 */
/* building api (allocating) */
tex_builder_t texture(int w, int h);

/* scope api */
#define scope_tex_build(tex, builder) _scope_tex_build_threaded(tex,builder,0,1)
#define scope_tex_rect(x,y,h,w)       _scope_tex_rect(x,y,h,w)

#define scope_rectcut_top(cut)        _scope_rectcut_top(cut)
#define scope_rectcut_left(cut)       _scope_rectcut_left(cut)
#define scope_rectcut_right(cut)      _scope_rectcut_right(cut)
#define scope_rectcut_bottom(cut)     _scope_rectcut_bottom(cut)

/* drawing api */
#define        color(...) temp = _color(temp, pixel_x, pixel_y, __VA_ARGS__)
tex_builder_t _color(tex_builder_t tex, int pixel_x, int pixel_y, color_t color);
#define        noise(...) temp = _noise(temp, pixel_x, pixel_y, __VA_ARGS__)
tex_builder_t _noise(tex_builder_t  tex, int pixel_x, int pixel_y, float intensity); /* TODO should take a color value */
#define        outline(color,thick) temp = _outline(temp, color, thick); _scope_tex_rect(thick,thick,temp.height-(thick*2),temp.width-(thick*2)) /* TODO why do we need (thick*2) here? */
tex_builder_t _outline(tex_builder_t tex, color_t color, unsigned int thickness);
/* for debugging */
#define        pixel(...) temp = _pixel(temp, ##__VA_ARGS__) // places a pixel at the current {x,y} start
tex_builder_t _pixel(tex_builder_t tex);

/* called by internally by macros */
tex_builder_t _set_mask(tex_builder_t* builder, unsigned int x, unsigned int y, unsigned int width, unsigned int height);

/* helper macros */
#define TOKEN_PASTE(a, b) a##b
#define CONCAT(a,b) TOKEN_PASTE(a,b)
#define UNIQUE_VAR(name) CONCAT(name, __LINE__)
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define CLAMP(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

/* helper functions */
static inline float step(float edge, float x) { return x >= edge ? 1.0f : 0.0f; }
static inline color_t alpha_blend(color_t src, color_t dst) {
    color_t blend;
    blend.r = CLAMP(src.r * src.a + dst.r * (1.0f - src.a), 0.0f, 1.0f);
    blend.g = CLAMP(src.g * src.a + dst.g * (1.0f - src.a), 0.0f, 1.0f);
    blend.b = CLAMP(src.b * src.a + dst.b * (1.0f - src.a), 0.0f, 1.0f);
    blend.a = 0.0f; // TODO for some reason we need to set it to 0 not see glClearColor...
    return blend;
}

static inline uint get_index(tex_builder_t texer, uint pixel_x, uint pixel_y) {
    #ifdef TEX_BUILDER_FLAG_FLIP
    return (texer.atlas_height - pixel_y - 1) * (texer.atlas_width) + pixel_x;
    #else
    return (pixel_y * texer.atlas_width) + pixel_x;
    #endif
}

#ifndef RUN_ON_COMPUTE_SHADER
#define _scope_for_every_pixel(thread_id, thread_count) \
    for (int pixel_x = thread_id; pixel_x < temp.atlas_width; pixel_x += thread_count) \
        for (int pixel_y = 0; pixel_y < temp.atlas_height; pixel_y++)
#else
/* NOTE: in a compute shader, we only use the for loop to sneak in the values for pixel_x and pixel_y */
#define _scope_for_every_pixel(thread_id, thread_count) \
    for (int pixel_x = gl_GlobalInvocationID.x, pixel_y = gl_GlobalInvocationID.y, UNIQUE_VAR(i) = 0; UNIQUE_VAR(i) == 0; UNIQUE_VAR(i)++ )
#endif

#define _scope_tex_build_threaded(tex, builder, thread_id, thread_count)                   \
    for (tex_builder_t temp = builder; temp.i == 0; (temp.i+=1, tex = _create(temp)))      \
        _scope_for_every_pixel(thread_id, thread_count)

#define _scope_tex_rect(x,y,w,h) \
    for (tex_builder_t UNIQUE_VAR(old_builder) = _set_mask(&temp, x,y,w,h); \
         UNIQUE_VAR(old_builder).i == 0;                                    \
         (temp = UNIQUE_VAR(old_builder), UNIQUE_VAR(old_builder).i+=1))

/* TODO test again */
#define _scope_rectcut_top(cut)    _scope_tex_rect(0, 0, cut, temp.width)
#define _scope_rectcut_left(cut)   _scope_tex_rect(0, 0, temp.height, cut)
#define _scope_rectcut_right(cut)  _scope_tex_rect((temp.width - cut), 0, temp.height, cut)
#define _scope_rectcut_bottom(cut) _scope_tex_rect(0, (temp.height - cut), cut, temp.width)

/* internal */
#ifdef TEX_BUILDER_IMPLEMENTATION
#include <stdlib.h> // for malloc & rand()
#include <math.h>   // for RAND_MAX, ...
#include <assert.h> // TODO take in assert macro from user
#include <time.h>   // for seeding srand()
tex_builder_t _color(tex_builder_t tex, int pixel_x, int pixel_y, color_t color) {
    float clip = step(tex.mask.x, pixel_x) * step(pixel_x, tex.mask.x + tex.mask.w) * step(tex.mask.y, pixel_y) * step(pixel_y, tex.mask.y + tex.mask.h);

    color.r *= clip;
    color.g *= clip;
    color.b *= clip;
    color.a *= clip;

    /* color the subtexture */
    size_t index = get_index(tex, pixel_x, pixel_y);
    tex.tex.rgb[index] = alpha_blend(color, tex.tex.rgb[index]);

    return tex;
}

tex_builder_t _noise(tex_builder_t texer, int pixel_x, int pixel_y, float intensity)  {
    float clip = step(texer.mask.x, pixel_x) * step(pixel_x, texer.mask.x + texer.mask.w) * step(texer.mask.y, pixel_y) * step(pixel_y, texer.mask.y + texer.mask.h);

    if (!(clip > 0.0f)) { return texer; }; /* NOTE: early out is actually faster on CPUs (still needs testing with shaders)  */

    size_t idx = get_index(texer, pixel_x, pixel_y);
    color_t noise;

    /* Add noise to each color component based on intensity */
    noise.r = texer.tex.rgb[idx].r + intensity * ((float)rand() / (float) RAND_MAX - 0.5f);
    noise.g = texer.tex.rgb[idx].g + intensity * ((float)rand() / (float) RAND_MAX - 0.5f);
    noise.b = texer.tex.rgb[idx].b + intensity * ((float)rand() / (float) RAND_MAX - 0.5f);

    /* clamp colors to [0, 1] and clip if needed */
    noise.r = (fminf(fmaxf(noise.r, 0.0f), 1.0f) * clip);
    noise.g = (fminf(fmaxf(noise.g, 0.0f), 1.0f) * clip);
    noise.b = (fminf(fmaxf(noise.b, 0.0f), 1.0f) * clip);
    noise.a = clip;

    texer.tex.rgb[idx] = alpha_blend(noise, texer.tex.rgb[idx]);

    return texer;
}
tex_builder_t _outline(tex_builder_t tex, color_t color, unsigned int thickness) {
    int x, y;

    //for (x = tex.x_start; x < tex.x_start + tex.width; x++) {
    //    for (y = tex.y_start; y < tex.y_start + thickness; y++) {
    //        tex.tex.rgb[y * tex.tex.width + x] = color; /* top side */
    //    }
    //    for (y = tex.y_start + tex.height - thickness; y < tex.y_start + tex.height; y++) {
    //        tex.tex.rgb[y * tex.tex.width + x] = color; /* bottom side */
    //    }
    //}

    //for (y = tex.y_start + thickness; y < tex.y_start + tex.height - thickness; y++) {
    //    for (x = tex.x_start; x < tex.x_start + thickness; x++) {
    //        tex.tex.rgb[y * tex.tex.width + x] = color; /* left side */
    //    }
    //    for (x = tex.x_start + tex.width - thickness; x < tex.x_start + tex.width; x++) {
    //        tex.tex.rgb[y * tex.tex.width + x] = color; /* right side */
    //    }
    //}

    return tex;
}

tex_builder_t texture(int w, int h) {
    tex_builder_t texer;

    /* init builder */
    texer.atlas_width  = w;
    texer.atlas_height = h;
    texer.i            = 0;

    /* set mask */
    texer.mask.x = 0;
    texer.mask.y = 0;
    texer.mask.w = w;
    texer.mask.h = h;

    /* init texture */
    texer.tex.width    = w;
    texer.tex.height   = h;
    texer.tex.rgb      = malloc(w * h * sizeof(color_t));

    return texer;
}

/* return copy of old builder, modify current builder's mask */
tex_builder_t _set_mask(tex_builder_t* builder, unsigned int x, unsigned int y, unsigned int width, unsigned int height) {
    tex_builder_t old = *builder;

    builder->mask.x = CLAMP(x + builder->mask.x, builder->mask.x, builder->mask.x + builder->mask.w); // NOTE: makes it so the rect is still visible for x outside of bounds
    builder->mask.y = CLAMP(y + builder->mask.y, builder->mask.y, builder->mask.y + builder->mask.h);

    /* TODO clamp w,h properly */
    builder->mask.w = min(width, builder->mask.w);
    builder->mask.h = min(height, builder->mask.h);

    return old;
}

tex_builder_t _pixel(tex_builder_t tex) {
   size_t index = get_index(tex, tex.mask.x, tex.mask.y);
   tex.tex.rgb[index] = (color_t){1,1,1,1};
   return tex;
}

/* NOTE: this is called for every single thread right now */
texture_t _create(tex_builder_t texer) {
    return texer.tex;
}
#endif
